/*
 * =========================================================================
 * FILENAME:   src/config/smart_contracts/base_layer.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    "Code is law – but law still needs memory allocation."
 *
 * PURPOSE:
 * This file provides the single, authoritative definition and memory
 * allocation for the `quicpro_smart_contracts_config` global variable.
 * =========================================================================
 */

#include "include/config/smart_contracts/base_layer.h"

/*
 * The global configuration instance for the smart-contracts subsystem.
 */
qp_smart_contracts_config_t quicpro_smart_contracts_config;
