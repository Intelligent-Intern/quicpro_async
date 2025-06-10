/*
 * src/tool_handler_registry.c – C Implementation for Managing Pipeline Tool Handlers
 * ===================================================================================
 *
 * This file implements the C-native logic for a registry that stores configurations
 * for all tools usable by the PipelineOrchestrator. It handles parsing PHP configuration
 * arrays into C structs and provides a fast lookup function for the orchestrator engine.
 *
 * The registry is populated via `quicpro_tool_handler_register_from_php`, which is
 * called by the corresponding PHP userland function.
 */

#include "php_quicpro.h"
#include "tool_handler_registry.h"
#include "cancel.h" /* For error throwing helpers */

#include <zend_API.h>
#include <zend_hash.h>
#include <zend_string.h>

/* --- Global Tool Handler Registry Definition --- */
static HashTable quicpro_tool_handler_registry;
static zend_bool quicpro_tool_registry_initialized = 0;

/* --- Static Helper Function Prototypes --- */
static int parse_mcp_target_config_from_php(HashTable *php_ht, quicpro_mcp_target_config_t *target_out, const char *context_for_error);
static int parse_rag_config_from_php(HashTable *php_ht, quicpro_rag_config_t *rag_out, const char *tool_name_for_error);
static void destroy_mcp_target_config_contents(quicpro_mcp_target_config_t *target);
static void destroy_rag_config_contents(quicpro_rag_config_t *rag);
static void tool_handler_config_dtor_internal(zval *pData);
static quicpro_tool_handler_config_t* create_tool_handler_from_php(const char *tool_name, HashTable *config_ht);


/* --- Memory Management: Destructor Implementations --- */

/**
 * @brief Frees memory allocated for the contents of an MCP target configuration struct.
 * @param target The MCP target config struct whose contents are to be freed.
 */
static void destroy_mcp_target_config_contents(quicpro_mcp_target_config_t *target) {
    if (!target) return;
    if (target->host) efree(target->host);
    if (target->service_name) efree(target->service_name);
    if (target->method_name) efree(target->method_name);
    if (Z_TYPE(target->mcp_client_options_php_array) != IS_UNDEF) {
        zval_ptr_dtor(&target->mcp_client_options_php_array);
    }
}

/**
 * @brief Frees memory allocated for the contents of a RAG configuration struct.
 * @param rag The RAG config struct whose contents are to be freed.
 */
static void destroy_rag_config_contents(quicpro_rag_config_t *rag) {
    if (!rag) return;
    destroy_mcp_target_config_contents(&rag->rag_agent_target);
    if (rag->enabled_param_key) efree(rag->enabled_param_key);
    if (rag->request_proto_schema) efree(rag->request_proto_schema);
    if (rag->response_proto_schema) efree(rag->response_proto_schema);
    if (rag->context_output_field_in_rag_response) efree(rag->context_output_field_in_rag_response);
    if (rag->target_context_field_in_llm_request) efree(rag->target_context_field_in_llm_request);
    if (rag->topics_from_param_key) efree(rag->topics_from_param_key);
    if (rag->topics_from_previous_step.source_tool_name_or_id) efree(rag->topics_from_previous_step.source_tool_name_or_id);
    if (rag->topics_from_previous_step.source_field_name) efree(rag->topics_from_previous_step.source_field_name);
    if (rag->rag_param_map) {
        zend_hash_destroy(rag->rag_param_map);
        efree(rag->rag_param_map);
    }
}

/**
 * @brief The HashTable destructor for a complete tool handler configuration.
 * @param pData A zval containing a pointer to the quicpro_tool_handler_config_t struct.
 */
static void tool_handler_config_dtor_internal(zval *pData) {
    quicpro_tool_handler_config_t *handler = (quicpro_tool_handler_config_t *)Z_PTR_P(pData);
    if (handler) {
        if (handler->tool_name) efree(handler->tool_name);
        destroy_mcp_target_config_contents(&handler->mcp_target);
        if (handler->input_proto_schema) efree(handler->input_proto_schema);
        if (handler->output_proto_schema) efree(handler->output_proto_schema);
        if (handler->param_map) {
            zend_hash_destroy(handler->param_map);
            efree(handler->param_map);
        }
        if (handler->output_map) {
            zend_hash_destroy(handler->output_map);
            efree(handler->output_map);
        }
        if (handler->rag_config) {
            destroy_rag_config_contents(handler->rag_config);
            efree(handler->rag_config);
        }
        efree(handler);
    }
}


/* --- Registry Lifecycle & Lookup Functions --- */

int quicpro_tool_handler_registry_init(void) {
    if (quicpro_tool_registry_initialized) return SUCCESS;
    zend_hash_init(&quicpro_tool_handler_registry, 0, NULL, tool_handler_config_dtor_internal, 1);
    quicpro_tool_registry_initialized = 1;
    return SUCCESS;
}

void quicpro_tool_handler_registry_shutdown(void) {
    if (quicpro_tool_registry_initialized) {
        zend_hash_destroy(&quicpro_tool_handler_registry);
        quicpro_tool_registry_initialized = 0;
    }
}

const quicpro_tool_handler_config_t* quicpro_tool_handler_get(const char *tool_name) {
    if (!quicpro_tool_registry_initialized) return NULL;
    return (quicpro_tool_handler_config_t *)zend_hash_str_find_ptr(&quicpro_tool_handler_registry, tool_name, strlen(tool_name));
}


/* --- Static C Helper Parsers --- */

/**
 * @brief Parses a PHP array into a quicpro_mcp_target_config_t C struct.
 * @param php_ht The source PHP HashTable.
 * @param target_out Pointer to the C struct to populate.
 * @param context_for_error A string (e.g., tool name) to include in error messages.
 * @return SUCCESS or FAILURE.
 */
static int parse_mcp_target_config_from_php(HashTable *php_ht, quicpro_mcp_target_config_t *target_out, const char *context_for_error) {
    zval *zv_temp;
    memset(target_out, 0, sizeof(quicpro_mcp_target_config_t));
    ZVAL_UNDEF(&target_out->mcp_client_options_php_array);

    if (!(zv_temp = zend_hash_str_find(php_ht, "host", sizeof("host")-1)) || Z_TYPE_P(zv_temp) != IS_STRING) {
        throw_pipeline_error_as_php_exception(0, "Invalid 'mcp_target' for '%s': missing 'host' string.", context_for_error);
        return FAILURE;
    }
    target_out->host = estrndup(Z_STRVAL_P(zv_temp), Z_STRLEN_P(zv_temp));

    if (!(zv_temp = zend_hash_str_find(php_ht, "port", sizeof("port")-1)) || Z_TYPE_P(zv_temp) != IS_LONG) {
        throw_pipeline_error_as_php_exception(0, "Invalid 'mcp_target' for '%s': missing 'port' integer.", context_for_error);
        return FAILURE;
    }
    target_out->port = Z_LVAL_P(zv_temp);

    if (!(zv_temp = zend_hash_str_find(php_ht, "service_name", sizeof("service_name")-1)) || Z_TYPE_P(zv_temp) != IS_STRING) {
        throw_pipeline_error_as_php_exception(0, "Invalid 'mcp_target' for '%s': missing 'service_name' string.", context_for_error);
        return FAILURE;
    }
    target_out->service_name = estrndup(Z_STRVAL_P(zv_temp), Z_STRLEN_P(zv_temp));

    if (!(zv_temp = zend_hash_str_find(php_ht, "method_name", sizeof("method_name")-1)) || Z_TYPE_P(zv_temp) != IS_STRING) {
        throw_pipeline_error_as_php_exception(0, "Invalid 'mcp_target' for '%s': missing 'method_name' string.", context_for_error);
        return FAILURE;
    }
    target_out->method_name = estrndup(Z_STRVAL_P(zv_temp), Z_STRLEN_P(zv_temp));

    if ((zv_temp = zend_hash_str_find(php_ht, "mcp_options", sizeof("mcp_options")-1)) && Z_TYPE_P(zv_temp) == IS_ARRAY) {
        ZVAL_COPY(&target_out->mcp_client_options_php_array, zv_temp);
    }
    return SUCCESS;
}

/**
 * @brief Parses the 'rag_config' section of a tool handler's PHP configuration.
 * @param php_ht The 'rag_config' PHP HashTable.
 * @param rag_out Pointer to the C struct to populate.
 * @param tool_name_for_error The tool name, for inclusion in error messages.
 * @return SUCCESS or FAILURE.
 */
static int parse_rag_config_from_php(HashTable *php_ht, quicpro_rag_config_t *rag_out, const char *tool_name_for_error) {
    zval *zv_temp;
    memset(rag_out, 0, sizeof(quicpro_rag_config_t));

    /* Parse RAG Agent MCP Target (required within rag_config) */
    if (!(zv_temp = zend_hash_str_find(php_ht, "mcp_target", sizeof("mcp_target")-1)) || Z_TYPE_P(zv_temp) != IS_ARRAY) {
         throw_pipeline_error_as_php_exception(0, "Invalid 'rag_config' for tool '%s': missing 'mcp_target' array for RAG agent.", tool_name_for_error);
        return FAILURE;
    }
    if (parse_mcp_target_config_from_php(Z_ARRVAL_P(zv_temp), &rag_out->rag_agent_target, "RAG agent") == FAILURE) return FAILURE;

    /* Parse all other required and optional string fields */
#define PARSE_RAG_STRING_FIELD(key) \
    if ((zv_temp = zend_hash_str_find(php_ht, #key, sizeof(#key)-1)) && Z_TYPE_P(zv_temp) == IS_STRING) { \
        rag_out->key = estrndup(Z_STRVAL_P(zv_temp), Z_STRLEN_P(zv_temp)); \
    } else { \
        throw_pipeline_error_as_php_exception(0, "Invalid 'rag_config' for tool '%s': missing or invalid string for '" #key "'.", tool_name_for_error); \
        return FAILURE; \
    }
    PARSE_RAG_STRING_FIELD(enabled_param_key);
    PARSE_RAG_STRING_FIELD(request_proto_schema);
    PARSE_RAG_STRING_FIELD(response_proto_schema);
    PARSE_RAG_STRING_FIELD(context_output_field_in_rag_response);
    PARSE_RAG_STRING_FIELD(target_context_field_in_llm_request);
#undef PARSE_RAG_STRING_FIELD

    if ((zv_temp = zend_hash_str_find(php_ht, "topics_from_param_key", sizeof("topics_from_param_key")-1)) && Z_TYPE_P(zv_temp) == IS_STRING) {
        rag_out->topics_from_param_key = estrndup(Z_STRVAL_P(zv_temp), Z_STRLEN_P(zv_temp));
    }
    if ((zv_temp = zend_hash_str_find(php_ht, "topics_from_previous_step_output", sizeof("topics_from_previous_step_output")-1)) && Z_TYPE_P(zv_temp) == IS_ARRAY) {
        zval* zv_tool = zend_hash_str_find(Z_ARRVAL_P(zv_temp), "tool_name", sizeof("tool_name")-1);
        zval* zv_field = zend_hash_str_find(Z_ARRVAL_P(zv_temp), "field_name", sizeof("field_name")-1);
        if (zv_tool && Z_TYPE_P(zv_tool) == IS_STRING && zv_field && Z_TYPE_P(zv_field) == IS_STRING) {
            rag_out->topics_from_previous_step.source_tool_name_or_id = estrndup(Z_STRVAL_P(zv_tool), Z_STRLEN_P(zv_tool));
            rag_out->topics_from_previous_step.source_field_name = estrndup(Z_STRVAL_P(zv_field), Z_STRLEN_P(zv_field));
        }
    }
    if ((zv_temp = zend_hash_str_find(php_ht, "rag_param_map", sizeof("rag_param_map")-1)) && Z_TYPE_P(zv_temp) == IS_ARRAY) {
        rag_out->rag_param_map = emalloc(sizeof(HashTable));
        zend_hash_init(rag_out->rag_param_map, 0, NULL, ZVAL_PTR_DTOR, 0);
        zend_hash_copy(rag_out->rag_param_map, Z_ARRVAL_P(zv_temp), (copy_ctor_func_t)zval_add_ref);
    }

    return SUCCESS;
}


/**
 * @brief Parses the full PHP configuration array for a tool handler into the C struct.
 * @param tool_name The name of the tool being registered.
 * @param config_ht The PHP HashTable of the configuration.
 * @return A pointer to a newly allocated quicpro_tool_handler_config_t, or NULL on failure.
 */
static quicpro_tool_handler_config_t* create_tool_handler_from_php(const char *tool_name, HashTable *config_ht) {
    quicpro_tool_handler_config_t *handler = ecalloc(1, sizeof(quicpro_tool_handler_config_t));
    zval *zv_temp;

    handler->tool_name = estrdup(tool_name);

    if (!(zv_temp = zend_hash_str_find(config_ht, "mcp_target", sizeof("mcp_target")-1)) || Z_TYPE_P(zv_temp) != IS_ARRAY) {
        throw_pipeline_error_as_php_exception(0, "Tool handler for '%s' is missing required 'mcp_target' array.", tool_name);
        goto error;
    }
    if (parse_mcp_target_config_from_php(Z_ARRVAL_P(zv_temp), &handler->mcp_target, tool_name) == FAILURE) goto error;

    if (!(zv_temp = zend_hash_str_find(config_ht, "input_proto_schema", sizeof("input_proto_schema")-1)) || Z_TYPE_P(zv_temp) != IS_STRING) {
        throw_pipeline_error_as_php_exception(0, "Tool handler for '%s' is missing required 'input_proto_schema' string.", tool_name);
        goto error;
    }
    handler->input_proto_schema = estrndup(Z_STRVAL_P(zv_temp), Z_STRLEN_P(zv_temp));

    if (!(zv_temp = zend_hash_str_find(config_ht, "output_proto_schema", sizeof("output_proto_schema")-1)) || Z_TYPE_P(zv_temp) != IS_STRING) {
        throw_pipeline_error_as_php_exception(0, "Tool handler for '%s' is missing required 'output_proto_schema' string.", tool_name);
        goto error;
    }
    handler->output_proto_schema = estrndup(Z_STRVAL_P(zv_temp), Z_STRLEN_P(zv_temp));

    if ((zv_temp = zend_hash_str_find(config_ht, "param_map", sizeof("param_map")-1)) && Z_TYPE_P(zv_temp) == IS_ARRAY) {
        handler->param_map = emalloc(sizeof(HashTable));
        zend_hash_init(handler->param_map, 0, NULL, ZVAL_PTR_DTOR, 0);
        zend_hash_copy(handler->param_map, Z_ARRVAL_P(zv_temp), (copy_ctor_func_t)zval_add_ref);
    }
    if ((zv_temp = zend_hash_str_find(config_ht, "output_map", sizeof("output_map")-1)) && Z_TYPE_P(zv_temp) == IS_ARRAY) {
        handler->output_map = emalloc(sizeof(HashTable));
        zend_hash_init(handler->output_map, 0, NULL, ZVAL_PTR_DTOR, 0);
        zend_hash_copy(handler->output_map, Z_ARRVAL_P(zv_temp), (copy_ctor_func_t)zval_add_ref);
    }

    if ((zv_temp = zend_hash_str_find(config_ht, "rag_config", sizeof("rag_config")-1)) && Z_TYPE_P(zv_temp) == IS_ARRAY) {
        handler->rag_config = ecalloc(1, sizeof(quicpro_rag_config_t));
        if (parse_rag_config_from_php(Z_ARRVAL_P(zv_temp), handler->rag_config, tool_name) == FAILURE) {
            goto error;
        }
    }

    return handler;

error:
    /* Comprehensive cleanup of partially created handler on parsing failure */
    if (handler) {
        /*
         * A full destructor call is not safe as some members might not be initialized.
         * We call our dedicated cleanup function which is designed for this.
         * The tool_handler_config_dtor_internal function is designed for fully
         * constructed objects. A separate cleanup for partials is better.
         */
        if (handler->tool_name) efree(handler->tool_name);
        destroy_mcp_target_config_contents(&handler->mcp_target);
        if (handler->input_proto_schema) efree(handler->input_proto_schema);
        if (handler->output_proto_schema) efree(handler->output_proto_schema);
        if (handler->param_map) { zend_hash_destroy(handler->param_map); efree(handler->param_map); }
        if (handler->output_map) { zend_hash_destroy(handler->output_map); efree(handler->output_map); }
        if (handler->rag_config) { destroy_rag_config_contents(handler->rag_config); efree(handler->rag_config); }
        efree(handler);
    }
    return NULL;
}


/* --- C Implementation of PHP-Exposed Registration Function --- */

int quicpro_tool_handler_register_from_php(const char *tool_name, zval *config_php_array) {
    if (!quicpro_tool_registry_initialized) {
        throw_pipeline_error_as_php_exception(0, "Tool handler registry not initialized.");
        return FAILURE;
    }
    if (zend_hash_str_exists(&quicpro_tool_handler_registry, tool_name, strlen(tool_name))) {
        throw_pipeline_error_as_php_exception(0, "Tool handler for '%s' is already registered.", tool_name);
        return FAILURE;
    }

    quicpro_tool_handler_config_t *handler = create_tool_handler_from_php(tool_name, Z_ARRVAL_P(config_php_array));
    if (!handler) {
        /* Exception already thrown */
        return FAILURE;
    }

    if (zend_hash_str_add_ptr(&quicpro_tool_handler_registry, tool_name, strlen(tool_name), handler) == NULL) {
        /* OOM or internal error */
        tool_handler_config_dtor_internal((zval*)(void*)&handler);
        throw_pipeline_error_as_php_exception(0, "Failed to add handler for '%s' to registry (internal error).", tool_name);
        return FAILURE;
    }

    return SUCCESS;
}

