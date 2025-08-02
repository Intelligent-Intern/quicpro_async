/*
 * =========================================================================
 * FILENAME:   src/config/iibin/base_layer.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    This must be Thursday. I never could get the hang of Thursdays.
 *
 * PURPOSE:
 * This file provides the single, authoritative definition and memory
 * allocation for the `quicpro_iibin_config` global variable.
 * =========================================================================
 */
#include "include/config/iibin/base_layer.h"

/*
 * This is the actual definition and memory allocation for the module's
 * configuration structure.
 */
qp_iibin_config_t quicpro_iibin_config;
