/*
 * =========================================================================
 * FILENAME:   include/config/mcp_and_orchestrator/default.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    There are... four... lights!
 *
 * PURPOSE:
 * This header declares the function for loading the module's hardcoded
 * default values for the MCP & Orchestrator engine.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_MCP_ORCHESTRATOR_DEFAULT_H
#define QUICPRO_CONFIG_MCP_ORCHESTRATOR_DEFAULT_H

/**
 * @brief Loads the hardcoded, default values into the module's config struct.
 */
void qp_config_mcp_and_orchestrator_defaults_load(void);

#endif /* QUICPRO_CONFIG_MCP_ORCHESTRATOR_DEFAULT_H */
