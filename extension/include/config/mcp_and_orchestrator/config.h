/*
 * =========================================================================
 * FILENAME:   include/config/mcp_and_orchestrator/config.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Engage.
 *
 * PURPOSE:
 * This header file declares the public C-API for applying runtime
 * configuration changes from PHP userland to the MCP & Orchestrator module.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_MCP_ORCHESTRATOR_CONFIG_H
#define QUICPRO_CONFIG_MCP_ORCHESTRATOR_CONFIG_H

#include "php.h"

/**
 * @brief Applies runtime configuration settings from a PHP array.
 * @param config_arr A zval pointer to a PHP array containing the key-value
 * pairs of the configuration to apply.
 * @return `SUCCESS` if all settings were successfully validated and applied,
 * `FAILURE` otherwise.
 */
int qp_config_mcp_and_orchestrator_apply_userland_config(zval *config_arr);

#endif /* QUICPRO_CONFIG_MCP_ORCHESTRATOR_CONFIG_H */
