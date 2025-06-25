/*
 * =========================================================================
 * FILENAME:   include/config/mcp_and_orchestrator/index.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Make it so.
 *
 * PURPOSE:
 * This header file declares the public C-API for the MCP & Orchestrator
 * configuration module.
 *
 * ARCHITECTURE:
 * It provides the function prototypes for the module's lifecycle, which
 * are called by the master dispatcher to orchestrate loading of settings.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_MCP_ORCHESTRATOR_INDEX_H
#define QUICPRO_CONFIG_MCP_ORCHESTRATOR_INDEX_H

/**
 * @brief Initializes the MCP & Orchestrator configuration module.
 */
void qp_config_mcp_and_orchestrator_init(void);

/**
 * @brief Shuts down the MCP & Orchestrator configuration module.
 */
void qp_config_mcp_and_orchestrator_shutdown(void);

#endif /* QUICPRO_CONFIG_MCP_ORCHESTRATOR_INDEX_H */
