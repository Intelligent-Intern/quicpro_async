/*
 * =========================================================================
 * FILENAME:   include/config/mcp_and_orchestrator/base_layer.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Things are only impossible until they're not.
 *
 * PURPOSE:
 * This header file defines the core data structure for the
 * `mcp_and_orchestrator` configuration module.
 *
 * ARCHITECTURE:
 * This struct holds the configuration values for the Model Context
 * Protocol (MCP) and the Pipeline Orchestrator engine.
 * =========================================================================
 */
#ifndef QUICPRO_CONFIG_MCP_ORCHESTRATOR_BASE_H
#define QUICPRO_CONFIG_MCP_ORCHESTRATOR_BASE_H

#include "php.h"
#include <stdbool.h>

typedef struct _qp_mcp_orchestrator_config_t {
    /* --- MCP (Model Context Protocol) Settings --- */
    zend_long mcp_default_request_timeout_ms;
    zend_long mcp_max_message_size_bytes;
    bool mcp_default_retry_policy_enable;
    zend_long mcp_default_retry_max_attempts;
    zend_long mcp_default_retry_backoff_ms_initial;
    bool mcp_enable_request_caching;
    zend_long mcp_request_cache_ttl_sec;

    /* --- Pipeline Orchestrator Settings --- */
    zend_long orchestrator_default_pipeline_timeout_ms;
    zend_long orchestrator_max_recursion_depth;
    zend_long orchestrator_loop_concurrency_default;
    bool orchestrator_enable_distributed_tracing;

} qp_mcp_orchestrator_config_t;

/* The single instance of this module's configuration data */
extern qp_mcp_orchestrator_config_t quicpro_mcp_orchestrator_config;

#endif /* QUICPRO_CONFIG_MCP_ORCHESTRATOR_BASE_H */
