/*
 * include/tool_handler_registry.h – API for Managing Pipeline Tool Handlers
 * =========================================================================
 *
 * This header file defines the C-level structures and function prototypes for
 * registering, managing, and retrieving "tool handler" configurations used by
 * the C-native Quicpro\PipelineOrchestrator.
 *
 * A tool handler configuration provides all necessary information for the
 * orchestrator to invoke a specific MCP agent/service that implements a
 * generic "tool" (e.g., 'GenerateText', 'FetchUrlContent'). This includes
 * MCP target details, Proto schema names for requests and responses, mappings
 * for parameters and results, and specific RAG (Retrieval Augmented Generation)
 * configurations if applicable.
 *
 * These configurations are typically populated from PHP userland via a static
 * method like `Quicpro\PipelineOrchestrator::registerToolHandler()`.
 */

#ifndef QUICPRO_TOOL_HANDLER_REGISTRY_H
#define QUICPRO_TOOL_HANDLER_REGISTRY_H

#include <php.h>        /* Zend API, zval, HashTable */
#include "mcp.h"        /* For quicpro_mcp_options_t or similar if MCP options are stored granularly */
                        /* Or simply use a HashTable* for mcp_options if they are passed as PHP arrays */

/* --- Forward Declarations --- */
/* Opaque struct for a compiled Proto schema representation, if needed at this level.
 * Alternatively, schema names (char*) might be sufficient if compilation/lookup
 * happens elsewhere. For now, assume schema names are used.
 */
/* typedef struct _quicpro_compiled_proto_schema_t quicpro_compiled_proto_schema_t; */


/* --- MCP Target Configuration --- */
/* Defines the specific MCP agent endpoint for a tool. */
typedef struct _quicpro_mcp_target_config_t {
    char *host;
    zend_long port;
    char *service_name;
    char *method_name;
    /*
     * Stores MCP client options specific to this target.
     * This zval would be a PHP array passed from registerToolHandler,
     * containing keys like 'tls_enable', 'connect_timeout_ms', etc.,
     * to be used when creating an MCP client instance for this tool.
     */
    zval mcp_client_options_php_array;
} quicpro_mcp_target_config_t;

/* --- Parameter and Output Mapping Configuration --- */
/* Defines how generic pipeline step parameters/outputs map to/from
 * the specific Proto schema fields of an MCP agent.
 * Represented as a HashTable mapping generic_name (char*) to specific_schema_field_name (char*).
 */
typedef HashTable quicpro_field_map_t; /* PHP: ['generic_param' => 'proto_field_name'] */

/* --- RAG (Retrieval Augmented Generation) Configuration --- */
/* Specific configuration for tools that integrate RAG before calling an LLM. */
typedef struct _quicpro_rag_config_t {
    zend_bool enabled_by_default;       /* If RAG is active unless explicitly disabled by 'enabled_param' */
    char *enabled_param_key;            /* Pipeline step param key to enable/disable RAG (e.g., "use_graph_context"). */
    quicpro_mcp_target_config_t rag_agent_target; /* MCP target for the GraphRAGAgent. */
    char *rag_request_proto_schema;     /* Proto schema name for requests to the GraphRAGAgent. */
    char *rag_response_proto_schema;    /* Proto schema name for responses from the GraphRAGAgent. */
    char *context_field_in_rag_response;/* Field in GraphRAGAgent's response containing the retrieved context text. */
    char *target_context_field_in_llm_request; /* Field in the main LLM agent's request schema to inject the RAG context. */

    char *topics_from_param_key;        /* Pipeline step param key for explicit topics (e.g., "context_topics_list"). */
    struct {                            /* Configuration to derive topics from a previous pipeline step's output. */
        char *source_tool_name_or_id;   /* Name/ID of the previous step providing topics (e.g., "ExtractKeywords"). */
        char *source_field_name;        /* Field in the previous step's output containing topics (e.g., "keywords"). */
    } topics_from_previous_step;

    quicpro_field_map_t *rag_param_map; /* Maps pipeline step params (e.g. 'context_depth') to GraphRAGAgent request fields. */
                                        /* Stored as a pointer to a HashTable. */
} quicpro_rag_config_t;

/* --- Tool Handler Configuration --- */
/* Represents the complete configuration for a single named tool. */
typedef struct _quicpro_tool_handler_config_t {
    char *tool_name;                     /* The generic name of the tool (e.g., "GenerateText"). */
    quicpro_mcp_target_config_t mcp_target; /* MCP agent that implements this tool. */
    char *input_proto_schema;            /* Proto schema name for requests to this tool's MCP agent. */
    char *output_proto_schema;           /* Proto schema name for responses from this tool's MCP agent. */
    quicpro_field_map_t *param_map;      /* Maps generic pipeline parameters to specific input_proto_schema fields. */
                                         /* Stored as a pointer to a HashTable. */
    quicpro_field_map_t *output_map;     /* Maps output_proto_schema fields back to generic pipeline output fields. */
                                         /* Stored as a pointer to a HashTable. */
    quicpro_rag_config_t *rag_config;    /* Optional RAG configuration for this tool. NULL if not applicable. */
                                         /* Stored as a pointer. */
} quicpro_tool_handler_config_t;


/* --- Registry API Functions (to be implemented in tool_handler_registry.c) --- */

/*
 * quicpro_tool_handler_registry_init()
 * ------------------------------------
 * Initializes the global tool handler registry (e.g., creates a global HashTable).
 * Called during MINIT of the extension.
 *
 * Returns: SUCCESS or FAILURE.
 */
int quicpro_tool_handler_registry_init(void);

/*
 * quicpro_tool_handler_registry_shutdown()
 * ----------------------------------------
 * Frees all resources associated with the global tool handler registry.
 * Called during MSHUTDOWN of the extension.
 */
void quicpro_tool_handler_registry_shutdown(void);

/*
 * quicpro_tool_handler_register_from_php(const char *tool_name, zval *config_php_array)
 * ------------------------------------------------------------------------------------
 * Parses a PHP configuration array and registers a tool handler.
 * This is the C function called by `Quicpro\PipelineOrchestrator::registerToolHandler()` in PHP.
 * It populates a quicpro_tool_handler_config_t struct and stores it.
 *
 * Parameters:
 * - tool_name: The name of the tool to register.
 * - config_php_array: A zval of type IS_ARRAY containing the configuration
 * (mcp_target, schemas, param_map, output_map, rag_config).
 *
 * Returns: SUCCESS if registration is successful, FAILURE otherwise.
 * Manages memory for all created C strings and structures.
 */
int quicpro_tool_handler_register_from_php(const char *tool_name, zval *config_php_array);

/*
 * quicpro_tool_handler_get(const char *tool_name)
 * -----------------------------------------------
 * Retrieves a read-only pointer to the configuration for a named tool.
 * The PipelineOrchestrator C code uses this to get instructions for executing a tool.
 *
 * Parameters:
 * - tool_name: The name of the tool.
 *
 * Returns: A const pointer to `quicpro_tool_handler_config_t` if found, NULL otherwise.
 * The returned pointer is owned by the registry and should not be freed by the caller.
 */
const quicpro_tool_handler_config_t* quicpro_tool_handler_get(const char *tool_name);

/*
 * Note: PHP_FUNCTION prototypes for user-facing static methods like
 * `Quicpro\PipelineOrchestrator::registerToolHandler()` are declared in
 * `php_quicpro_arginfo.h` and their C implementations (which would call
 * `quicpro_tool_handler_register_from_php`) are in `php_quicpro.c` or
 * a dedicated `pipeline_orchestrator_php_api.c`.
 * This header focuses on the C-level registry management.
 */

#endif /* QUICPRO_TOOL_HANDLER_REGISTRY_H */