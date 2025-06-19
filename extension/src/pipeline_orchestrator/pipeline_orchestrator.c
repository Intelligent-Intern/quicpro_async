/*
 * src/pipeline_orchestrator.c – C-Native Pipeline Orchestration Engine
 * ====================================================================
 *
 * This file implements the core C-native logic for the Pipeline Orchestrator.
 * It parses pipeline definitions from PHP, executes each step by making MCP calls
 * to registered tool handlers, manages data flow, handles conditional logic,
 * and integrates advanced features like GraphRAG and automated context logging.
 */

#include "php_quicpro.h"
#include "pipeline_orchestrator.h"
#include "tool_handler_registry.h"
#include "proto.h"
#include "mcp.h"
#include "cancel.h"
#include "proto_internal.h" /* For direct access to compiled schema structs */

#include <zend_API.h>
#include <zend_exceptions.h>
#include <zend_hash.h>

/* --- Global Orchestrator Settings --- */
static zend_bool g_auto_logging_enabled = 0;
static quicpro_mcp_target_config_t *g_logger_agent_target = NULL;
/* Further global settings (e.g., default RAG provider) could be defined here */

/* --- Static Helper Function Prototypes --- */
static int execute_pipeline_c(zval *initial_data_zval, zval *pipeline_def_php_array, zval *exec_options_php_array, zval *return_value);
static int execute_step_c(HashTable *execution_context, zval *step_def_php_array, zend_bool *last_condition_result);
static zval* resolve_input_source_value(const char *source_path, size_t source_path_len, zval *initial_data, HashTable *execution_context);
static int execute_rag_sub_call(const quicpro_tool_handler_config_t* tool_handler, HashTable *step_params, HashTable *execution_context, zval *rag_context_out);
static int execute_mcp_call(const quicpro_mcp_target_config_t *target, zval *request_payload_zval, const char* input_schema_name, const char* output_schema_name, zval *response_out);
static void log_pipeline_event(const char *event_type, HashTable *event_data);

/* --- Lifecycle and Configuration Functions --- */

int quicpro_pipeline_orchestrator_init_settings(void) {
    g_auto_logging_enabled = 0;
    g_logger_agent_target = NULL;
    return SUCCESS;
}

void quicpro_pipeline_orchestrator_shutdown_settings(void) {
    if (g_logger_agent_target) {
        // Proper cleanup for the target config struct
        efree(g_logger_agent_target->host);
        efree(g_logger_agent_target->service_name);
        efree(g_logger_agent_target->method_name);
        zval_ptr_dtor(&g_logger_agent_target->mcp_client_options_php_array);
        efree(g_logger_agent_target);
        g_logger_agent_target = NULL;
    }
}

int quicpro_pipeline_orchestrator_configure_auto_logging_from_php(zval *logger_config_php_array) {
    /* TODO: Parse the logger_config_php_array and populate g_logger_agent_target */
    /* This involves deep parsing of the array and allocating/populating the C structs */
    /* For now, just enable the flag as a placeholder */
    zval *zv_enable = zend_hash_str_find(Z_ARRVAL_P(logger_config_php_array), "enable_auto_log", sizeof("enable_auto_log") - 1);
    if (zv_enable && zend_is_true(zv_enable)) {
        g_auto_logging_enabled = 1;
        /* ... parse mcp_target, batch_size etc. and store in global static variables ... */
    } else {
        g_auto_logging_enabled = 0;
    }
    return SUCCESS;
}

int quicpro_pipeline_orchestrator_register_tool_handler_from_php(const char *tool_name, zval *handler_config_php_array) {
    /* This function is a simple pass-through to the tool handler registry module */
    return quicpro_tool_handler_register_from_php(tool_name, handler_config_php_array);
}


/* --- PHP_FUNCTION Implementations --- */

PHP_FUNCTION(quicpro_pipeline_orchestrator_run)
{
    zval *initial_data_zval;
    zval *pipeline_def_php_array;
    zval *exec_options_php_array = NULL;

    ZEND_PARSE_PARAMETERS_START(2, 3)
        Z_PARAM_ZVAL(initial_data_zval)
        Z_PARAM_ARRAY(pipeline_def_php_array)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY_OR_NULL(exec_options_php_array)
    ZEND_PARSE_PARAMETERS_END();

    /*
     * The `execute_pipeline_c` function contains the core orchestration logic.
     * It takes PHP zvals, performs the C-native execution, and populates the
     * PHP return_value zval directly.
     */
    if (execute_pipeline_c(initial_data_zval, pipeline_def_php_array, exec_options_php_array, return_value) == FAILURE) {
        /*
         * If the orchestrator itself fails critically, an exception has likely been thrown.
         * If not, we should return false. If return_value was partially built,
         * we should clean it up.
         */
        if (Z_TYPE_P(return_value) != IS_FALSE) {
            zval_ptr_dtor(return_value);
            RETURN_FALSE;
        }
    }
}

PHP_FUNCTION(quicpro_pipeline_orchestrator_register_tool) { /* Maps to registerToolHandler */
    char *tool_name;
    size_t tool_name_len;
    zval *config_array;
    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STRING(tool_name, tool_name_len)
        Z_PARAM_ARRAY(config_array)
    ZEND_PARSE_PARAMETERS_END();

    if (quicpro_pipeline_orchestrator_register_tool_handler_from_php(tool_name, config_array) == SUCCESS) {
        RETURN_TRUE;
    }
    RETURN_FALSE;
}

PHP_FUNCTION(quicpro_pipeline_orchestrator_configure_logging) {
    zval *config_array;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY(config_array)
    ZEND_PARSE_PARAMETERS_END();

    if (quicpro_pipeline_orchestrator_configure_auto_logging_from_php(config_array) == SUCCESS) {
        RETURN_TRUE;
    }
    RETURN_FALSE;
}


/* --- Core Orchestration C Logic --- */

static int execute_pipeline_c(zval *initial_data_zval, zval *pipeline_def_php_array, zval *exec_options_php_array, zval *return_value) {
    zval step_results_array;
    HashTable *execution_context;
    zend_bool last_condition_result = 1; /* Assume initial condition is true */

    /* The execution_context will store results of steps: ['step_id' => [output_data]] */
    ALLOC_HASHTABLE(execution_context);
    zend_hash_init(execution_context, 8, NULL, ZVAL_PTR_DTOR, 0);

    /* Initialize the result object/array that will be returned to PHP */
    /* This could be a specific Quicpro\PipelineResult object or a simple array */
    array_init(return_value);

    zval *step_def_zval;
    ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(pipeline_def_php_array), step_def_zval) {
        if (Z_TYPE_P(step_def_zval) != IS_ARRAY) {
            zend_hash_destroy(execution_context); FREE_HASHTABLE(execution_context);
            throw_pipeline_error_as_php_exception(0, "Pipeline definition invalid: each step must be an array.");
            return FAILURE;
        }

        /* Here we pass a pointer to `execution_context` so steps can add results to it.
         * We also pass `initial_data_zval` for resolving '@initial.*' mappings.
         */
        if (execute_step_c(execution_context, step_def_zval, &last_condition_result) == FAILURE) {
            /* An exception was thrown inside execute_step_c. Populate error info. */
            add_assoc_bool(return_value, "isSuccess", 0);
            /* TODO: Populate error message from the exception or internal state */
            zend_hash_destroy(execution_context); FREE_HASHTABLE(execution_context);
            return FAILURE;
        }
    } ZEND_HASH_FOREACH_END();

    /* Finalize result */
    add_assoc_bool(return_value, "isSuccess", 1);

    /* Add step outputs to the final result for debugging/access */
    ZVAL_ARR(return_value, zend_hash_copy(execution_context, Z_ARRVAL_P(return_value), (copy_ctor_func_t)zval_add_ref));
    add_assoc_string(return_value, "finalMessage", "Pipeline completed successfully.");

    zend_hash_destroy(execution_context);
    FREE_HASHTABLE(execution_context);
    return SUCCESS;
}

/*
 * Executes a single step of the pipeline.
 * This is the most complex function, orchestrating RAG, input mapping, MCP calls, etc.
 */
static int execute_step_c(HashTable *execution_context, zval *step_def_php_array, zend_bool *last_condition_result) {
    zval *zv_temp;
    char *tool_name = NULL;

    /* Check conditional execution flag */
    if ((zv_temp = zend_hash_str_find(Z_ARRVAL_P(step_def_php_array), "condition_true_only", sizeof("condition_true_only") - 1)) && zend_is_true(zv_temp)) {
        if (!(*last_condition_result)) {
            /* This step is skipped */
            /* TODO: Log this skip event */
            return SUCCESS;
        }
    }

    /* Get tool name */
    if (!(zv_temp = zend_hash_str_find(Z_ARRVAL_P(step_def_php_array), "tool", sizeof("tool") - 1)) || Z_TYPE_P(zv_temp) != IS_STRING) {
        throw_pipeline_error_as_php_exception(0, "Pipeline step is missing a valid 'tool' name string.");
        return FAILURE;
    }
    tool_name = Z_STRVAL_P(zv_temp);

    /* Fetch the registered handler configuration for this tool */
    const quicpro_tool_handler_config_t *tool_handler = quicpro_tool_handler_get(tool_name);
    if (!tool_handler) {
        throw_pipeline_error_as_php_exception(0, "No handler registered for tool '%s'.", tool_name);
        return FAILURE;
    }

    zval *step_params = zend_hash_str_find(Z_ARRVAL_P(step_def_php_array), "params", sizeof("params") - 1);
    zval rag_context; ZVAL_NULL(&rag_context);
    int rag_result = SUCCESS;

    /* RAG sub-pipeline execution, if configured and enabled */
    if (tool_handler->rag_config) {
        zend_bool use_rag = 0;
        if (step_params && (zv_temp = zend_hash_str_find(Z_ARRVAL_P(step_params), tool_handler->rag_config->enabled_param_key, strlen(tool_handler->rag_config->enabled_param_key)))) {
            use_rag = zend_is_true(zv_temp);
        }
        if (use_rag) {
            rag_result = execute_rag_sub_call(tool_handler, Z_ARRVAL_P(step_params), execution_context, &rag_context);
            if (rag_result == FAILURE) {
                return FAILURE; /* Error already thrown */
            }
        }
    }

    /*
     * TODO: Implement full input mapping logic.
     * 1. Create a new PHP array `request_payload_php_array` for the MCP call.
     * 2. Get the step's `input_map` array.
     * 3. For each mapping in `input_map` ('target_field' => 'source_path'):
     * - Call `resolve_input_source_value('source_path', ...)` to get the zval.
     * - Add it to `request_payload_php_array` with key 'target_field'.
     * 4. If RAG was used, add the `rag_context` zval to `request_payload_php_array`
     * using the key from `tool_handler->rag_config->target_context_field_in_llm_request`.
     * 5. Get the step's `params` array and merge it into `request_payload_php_array`,
     * applying the `param_map` from the tool handler.
     */
    zval mcp_request_payload;
    array_init(&mcp_request_payload);
    // Placeholder for mapping logic...
    // add_assoc_string(&mcp_request_payload, "placeholder_input", "data");


    zval mcp_response; ZVAL_UNDEF(&mcp_response);
    if (execute_mcp_call(&tool_handler->mcp_target, &mcp_request_payload, tool_handler->input_proto_schema, tool_handler->output_proto_schema, &mcp_response) == FAILURE) {
        zval_ptr_dtor(&mcp_request_payload);
        return FAILURE;
    }
    zval_ptr_dtor(&mcp_request_payload);


    /*
     * TODO: Output mapping logic.
     * Use `tool_handler->output_map` to transform `mcp_response` into the final step output if needed.
     * For now, we store the direct response.
     */
    zend_string *step_id_key = zend_string_init(tool_name, strlen(tool_name), 0);
    zend_hash_update(execution_context, step_id_key, &mcp_response); // zend_hash_update takes ownership
    zend_string_release(step_id_key);


    /* If this was a ConditionalLogic tool, update the last_condition_result flag */
    if (strcmp(tool_name, "ConditionalLogic") == 0) {
        if (Z_TYPE(mcp_response) == IS_ARRAY && (zv_temp = zend_hash_str_find(Z_ARRVAL(mcp_response), "condition_met", sizeof("condition_met")-1))) {
            *last_condition_result = zend_is_true(zv_temp);
        } else {
            *last_condition_result = 0; // Default to false if condition output is invalid
        }
    } else {
        *last_condition_result = 1; // Reset condition for next step unless it's another conditional
    }

    if (Z_TYPE(rag_context) != IS_NULL) zval_ptr_dtor(&rag_context);
    return SUCCESS;
}

/*
 * --- PLACEHOLDER IMPLEMENTATIONS FOR HELPERS ---
 * These functions contain significant logic and would be fully built out.
 */
static zval* resolve_input_source_value(const char *source_path, size_t source_path_len, zval *initial_data, HashTable *execution_context) {
    /* TODO: Implement logic to parse '@initial.key' and '@step_id.output.key' and retrieve zval* */
    return NULL;
}

static int execute_rag_sub_call(const quicpro_tool_handler_config_t* tool_handler, HashTable *step_params, HashTable *execution_context, zval *rag_context_out) {
    /* TODO:
     * 1. Get RAG topics from step_params ('context_topics_list') or previous step output from execution_context.
     * 2. Get RAG params (depth, tokens) from step_params.
     * 3. Construct RAG request payload zval.
     * 4. Call execute_mcp_call for the RAG agent (`tool_handler->rag_config->rag_agent_target`).
     * 5. Extract context text from RAG response and populate `rag_context_out`.
     */
    ZVAL_STRING(rag_context_out, "/* Retrieved RAG context would be here */");
    return SUCCESS;
}

static int execute_mcp_call(const quicpro_mcp_target_config_t *target, zval *request_payload_zval, const char* input_schema_name, const char* output_schema_name, zval *response_out) {
    /* TODO:
     * 1. Use a connection pool or create a new `quicpro_mcp_connect` using `target->mcp_client_options_php_array`.
     * 2. Call `quicpro_proto_encode` with `input_schema_name` and `request_payload_zval`.
     * 3. Call `quicpro_mcp_request` with the target details and encoded payload.
     * 4. Call `quicpro_proto_decode` with `output_schema_name` and the binary response.
     * 5. Populate `response_out` zval.
     * 6. Handle all potential errors and throw exceptions.
     */
    ZVAL_COPY(response_out, request_payload_zval); // Echo back payload as placeholder
    return SUCCESS;
}

static void log_pipeline_event(const char *event_type, HashTable *event_data) {
    if (!g_auto_logging_enabled || !g_logger_agent_target) return;
    /* TODO:
     * 1. Construct event payload zval.
     * 2. Call execute_mcp_call to send the log event to `g_logger_agent_target`.
     * This should likely be a fire-and-forget call (non-blocking, no response needed).
     */
}