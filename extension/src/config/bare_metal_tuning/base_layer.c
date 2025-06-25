/*
 * =========================================================================
 * FILENAME:   src/config/bare_metal_tuning/base_layer.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Fiery the angels fell. Deep thunder rolled around their
 * shores... burning with the fires of Orc.
 *
 * PURPOSE:
 * This file provides the single, authoritative definition and memory
 * allocation for the `quicpro_bare_metal_config` global variable.
 * =========================================================================
 */
#include "include/config/bare_metal_tuning/base_layer.h"

/*
 * This is the actual definition and memory allocation for the module's
 * configuration structure.
 */
qp_bare_metal_config_t quicpro_bare_metal_config;
