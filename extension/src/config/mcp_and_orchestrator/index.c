/*
 * =========================================================================
 * FILENAME:   src/config/mcp_and_orchestrator/index.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Shaka, when the walls fell.
 *
 * PURPOSE:
 * This file implements the lifecycle functions for the MCP & Orchestrator
 * configuration module.
 * =========================================================================
 */

#include "include/config/mcp_and_orchestrator/index.h"
#include "include/config/mcp_and_orchestrator/default.h"
#include "include/config/mcp_and_orchestrator/ini.h"
#include "php.h"

/**
 * @brief Initializes the MCP & Orchestrator configuration module.
 */
void qp_config_mcp_and_orchestrator_init(void)
{
    /* Step 1: Load hardcoded, safe defaults. */
    qp_config_mcp_and_orchestrator_defaults_load();

    /* Step 2: Register INI handlers to override defaults. */
    qp_config_mcp_and_orchestrator_ini_register();
}

/**
 * @brief Shuts down the MCP & Orchestrator configuration module.
 */
void qp_config_mcp_and_orchestrator_shutdown(void)
{
    qp_config_mcp_and_orchestrator_ini_unregister();
}
