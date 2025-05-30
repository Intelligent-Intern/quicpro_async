/*
 * include/pipeline_orchestrator.h – C API for Native Pipeline Orchestration
 * =========================================================================
 *
 * This header file defines the C-level structures and function prototypes for
 * the native Pipeline Orchestrator engine within the php-quicpro_async extension.
 * The orchestrator executes pipelines defined in PHP, managing step execution,
 * data flow, MCP agent invocations, Proto serialization, and integration points
 * for advanced features like GraphRAG and automated context logging.
 *
 * PHP userland interacts with this C engine primarily through static methods
 * on the `Quicpro\PipelineOrchestrator` PHP class, which wrap these C functions.
 */

#ifndef QUICPRO_PIPELINE_ORCHESTRATOR_H
#define QUICPRO_PIPELINE_ORCHESTRATOR_H

#include <php.h>        /* Zend API, zval, HashTable */
#include "tool_handler_registry.h" /* For quicpro_tool_handler_config_t */
/* Other internal Quicpro headers for MCP client and Proto might be included
 * directly in pipeline_orchestrator.c rather than here, if not needed for
 * function signatures exposed in this public C API header.
 */

/* --- Forward Declarations --- */
/* Opaque structure for internal pipeline execution state if needed by any helper functions declared here. */
/* typedef struct _quicpro_pipeline_execution_context_t quicpro_pipeline_execution_context_t; */


/* --- C Structures for Pipeline Definition & Options (populated from PHP) --- */

/* Represents a single step in the pipeline definition. */
typedef struct _quicpro_pipeline_step_def_c {
    char *step_id_or_tool_name; /* User-defined ID for the step, or defaults to tool_name if unique.
                                   Used for referencing outputs, e.g., @step_id.output_field.
                                   The orchestrator can generate unique internal IDs if needed. */
    char *tool_name;            /* Generic tool name, e.g., "GenerateText", "FetchUrlContent". */
    zval params_php_array;      /* PHP associative array: Static parameters for the tool. Copied or ref-counted. */
    zval input_map_php_array;   /* PHP associative array: Maps tool input fields to context sources
                                   (e.g., ['proto_field' => '@initial.key', 'another_field' => '@previous_step_id.output_key.sub_field']).
                                   Copied or ref-counted. */
    zend_bool condition_true_only; /* If true, step only runs if the *immediately preceding* ConditionalLogic tool step
                                      (or a specially designated conditional output) evaluated to true. */
    /* char *condition_source_step_id; // Optional: More explicit control over which step's condition to check. */
} quicpro_pipeline_step_def_c;

/* Represents the overall pipeline definition. */
typedef struct _quicpro_pipeline_def_c {
    quicpro_pipeline_step_def_c *steps; /* Array of step definitions. */
    size_t num_steps;                   /* Number of steps in the pipeline. */
} quicpro_pipeline_def_c;

/* Options controlling the execution of a pipeline instance. */
typedef struct _quicpro_pipeline_exec_options_c {
    zend_long overall_timeout_ms; /* Timeout for the entire pipeline execution in milliseconds. 0 for no timeout. */
    zend_bool fail_fast;          /* If true, the pipeline stops immediately on the first step failure. */
                                  /* Add other execution-specific options here if needed. */
    /* zval default_mcp_options_override_php_array; // Optional PHP array to override MCP options for all steps in this run */
} quicpro_pipeline_exec_options_c;


/* --- Orchestrator Global Configuration Functions (called from PHP bootstrap) --- */

/*
 * quicpro_pipeline_orchestrator_init_settings()
 * ---------------------------------------------
 * Initializes global settings for the C-native Pipeline Orchestrator.
 * Called once during extension MINIT or application bootstrap.
 *
 * Returns: SUCCESS or FAILURE.
 */
int quicpro_pipeline_orchestrator_init_settings(void);

/*
 * quicpro_pipeline_orchestrator_shutdown_settings()
 * -------------------------------------------------
 * Cleans up global settings for the orchestrator.
 * Called during extension MSHUTDOWN.
 */
void quicpro_pipeline_orchestrator_shutdown_settings(void);

/*
 * quicpro_pipeline_orchestrator_configure_auto_logging_from_php(zval *logger_config_php_array)
 * -------------------------------------------------------------------------------------------
 * Configures the C-native orchestrator's automated context logging feature.
 * Called by `Quicpro\PipelineOrchestrator::enableAutoContextLogging()` in PHP.
 *
 * Parameters:
 * - logger_config_php_array: A PHP array detailing the MCP target for the GraphEventLoggerAgent,
 * batching options, log level, etc. (e.g., as in the example {'mcp_target':{...}, 'batch_size':100}).
 *
 * Returns: SUCCESS or FAILURE.
 */
int quicpro_pipeline_orchestrator_configure_auto_logging_from_php(zval *logger_config_php_array);

/*
 * quicpro_pipeline_orchestrator_register_tool_handler_from_php(const char *tool_name, zval *handler_config_php_array)
 * -------------------------------------------------------------------------------------------------------------------
 * C function called by PHP's `Quicpro\PipelineOrchestrator::registerToolHandler()`.
 * This function interfaces with the tool_handler_registry module.
 *
 * Parameters:
 * - tool_name: The generic name of the tool.
 * - handler_config_php_array: PHP array containing the tool's handler configuration.
 *
 * Returns: SUCCESS or FAILURE.
 */
int quicpro_pipeline_orchestrator_register_tool_handler_from_php(const char *tool_name, zval *handler_config_php_array);


/* --- Pipeline Execution Function (called from PHP `run` method) --- */

/*
 * quicpro_pipeline_orchestrator_run_from_php(zval *initial_data_php_zval, zval *pipeline_def_php_array, zval *exec_options_php_array, zval *return_value)
 * -------------------------------------------------------------------------------------------------------------------------------------------------------
 * The core C function called by `Quicpro\PipelineOrchestrator::run()` in PHP.
 * It takes PHP inputs, converts them to C structures, executes the pipeline,
 * and populates the PHP `return_value` with a result object/array.
 *
 * Parameters:
 * - initial_data_php_zval: A PHP zval (string, array, or object) representing the initial input to the pipeline.
 * - pipeline_def_php_array: A PHP array representing the pipeline definition (array of step definition arrays).
 * - exec_options_php_array: An optional PHP array for execution options. Can be NULL.
 * - return_value: The zval to be populated with the PHP `WorkflowResult` object (or an array).
 *
 * The C implementation will:
 * 1. Parse `pipeline_def_php_array` into `quicpro_pipeline_def_c`.
 * 2. Parse `exec_options_php_array` into `quicpro_pipeline_exec_options_c`.
 * 3. Execute the pipeline step-by-step:
 * a. Resolve tool handlers using `tool_handler_registry.h` API.
 * b. Manage an internal C-level execution context for data flow (`@initial`, `@previous`).
 * c. Handle input/output mapping (`param_map`, `output_map` from tool handler).
 * d. Perform RAG sub-calls if configured in the tool handler.
 * e. Make MCP calls using C-native MCP client.
 * f. Handle Proto encoding/decoding using C-native Proto functions.
 * g. Emit events for auto-logging if configured.
 * h. Handle `ConditionalLogic` and errors.
 * 4. Construct a PHP result (e.g., an object of a `Quicpro\PipelineResult` class or an associative array)
 * and place it in `return_value`.
 *
 * This function itself doesn't use PHP_FUNCTION directly; it's a helper called by the actual PHP_FUNCTION.
 * The PHP_FUNCTION(quicpro_pipeline_orchestrator_run) would wrap this call.
 */
/* The actual PHP_FUNCTION prototype will be in php_quicpro.h / php_quicpro_arginfo.h */
/* This is a conceptual C interface function that the PHP_FUNCTION would call: */
/*
int quicpro_orchestrator_execute_pipeline_c(
    quicpro_pipeline_def_c *pipeline_c_def,
    zval *initial_data_php_zval,
    quicpro_pipeline_exec_options_c *exec_c_options,
    zval *result_php_zval_out // To be populated by this function
);
*/

/* PHP_FUNCTIONs that will wrap the C logic: */

/*
 * PHP_FUNCTION(quicpro_pipeline_orchestrator_run)
 * (Declaration of the PHP-bindable function)
 */
PHP_FUNCTION(quicpro_pipeline_orchestrator_run);

/*
 * PHP_FUNCTION(quicpro_pipeline_orchestrator_register_tool)
 * (Declaration of the PHP-bindable function that calls quicpro_pipeline_orchestrator_register_tool_handler_from_php)
 */
PHP_FUNCTION(quicpro_pipeline_orchestrator_register_tool); // Matches common registerToolHandler

/*
 * PHP_FUNCTION(quicpro_pipeline_orchestrator_configure_logging)
 * (Declaration of the PHP-bindable function that calls quicpro_pipeline_orchestrator_configure_auto_logging_from_php)
 */
PHP_FUNCTION(quicpro_pipeline_orchestrator_configure_logging);


#endif /* QUICPRO_PIPELINE_ORCHESTRATOR_H */