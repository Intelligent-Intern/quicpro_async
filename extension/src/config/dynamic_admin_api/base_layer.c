/*
 * =========================================================================
 * FILENAME:   src/config/dynamic_admin_api/base_layer.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    But until that day, accept this justice as a gift on my
 * daughter's wedding day.
 *
 * PURPOSE:
 * This file provides the single, authoritative definition and memory
 * allocation for the `quicpro_dynamic_admin_api_config` global variable.
 * =========================================================================
 */
#include "include/config/dynamic_admin_api/base_layer.h"

/*
 * This is the actual definition and memory allocation for the module's
 * configuration structure.
 */
qp_dynamic_admin_api_config_t quicpro_dynamic_admin_api_config;
