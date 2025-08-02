/*
 * =========================================================================
 * FILENAME:   src/config/mcp_and_orchestrator/default.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    The needs of the many outweigh the needs of the few. Or the one.
 *
 * PURPOSE:
 * This file implements the function that loads the hardcoded, default
 * values into the MCP & Orchestrator configuration struct.
 * =========================================================================
 */

#include "include/config/mcp_and_orchestrator/default.h"
#include "include/config/mcp_and_orchestrator/base_layer.h"
#include "php.h"

/**
 * @brief Loads the hardcoded, default values into the module's config struct.
 */
void qp_config_mcp_and_orchestrator_defaults_load(void)
{
    /* --- MCP (Model Context Protocol) Settings --- */
    quicpro_mcp_orchestrator_config.mcp_default_request_timeout_ms = 30000;
    quicpro_mcp_orchestrator_config.mcp_max_message_size_bytes = 4194304; /* 4MB */
    quicpro_mcp_orchestrator_config.mcp_default_retry_policy_enable = true;
    quicpro_mcp_orchestrator_config.mcp_default_retry_max_attempts = 3;
    quicpro_mcp_orchestrator_config.mcp_default_retry_backoff_ms_initial = 100;
    quicpro_mcp_orchestrator_config.mcp_enable_request_caching = false;
    quicpro_mcp_orchestrator_config.mcp_request_cache_ttl_sec = 60;

    /* --- Pipeline Orchestrator Settings --- */
    quicpro_mcp_orchestrator_config.orchestrator_default_pipeline_timeout_ms = 120000;
    quicpro_mcp_orchestrator_config.orchestrator_max_recursion_depth = 10;
    quicpro_mcp_orchestrator_config.orchestrator_loop_concurrency_default = 50;
    quicpro_mcp_orchestrator_config.orchestrator_enable_distributed_tracing = true;
}
