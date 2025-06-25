/*
 * =========================================================================
 * FILENAME:   include/config/mcp_and_orchestrator/ini.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Tea, Earl Grey, hot.
 *
 * PURPOSE:
 * This header file declares the internal C-API for handling the `php.ini`
 * entries of the MCP & Orchestrator configuration module.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_MCP_ORCHESTRATOR_INI_H
#define QUICPRO_CONFIG_MCP_ORCHESTRATOR_INI_H

/**
 * @brief Registers this module's php.ini entries with the Zend Engine.
 */
void qp_config_mcp_and_orchestrator_ini_register(void);

/**
 * @brief Unregisters this module's php.ini entries from the Zend Engine.
 */
void qp_config_mcp_and_orchestrator_ini_unregister(void);

#endif /* QUICPRO_CONFIG_MCP_ORCHESTRATOR_INI_H */
