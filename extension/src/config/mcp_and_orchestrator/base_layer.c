/*
 * =========================================================================
 * FILENAME:   src/config/mcp_and_orchestrator/base_layer.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Darmok and Jalad at Tanagra.
 *
 * PURPOSE:
 * This file provides the single, authoritative definition and memory
 * allocation for the `quicpro_mcp_orchestrator_config` global variable.
 * =========================================================================
 */
#include "include/config/mcp_and_orchestrator/base_layer.h"

/*
 * This is the actual definition and memory allocation for the module's
 * configuration structure.
 */
qp_mcp_orchestrator_config_t quicpro_mcp_orchestrator_config;
