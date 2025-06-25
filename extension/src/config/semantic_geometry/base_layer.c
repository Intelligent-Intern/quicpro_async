/*
 * =========================================================================
 * FILENAME:   src/config/semantic_geometry/base_layer.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    And you see, I said, men passing along the wall carrying all
 * sorts of vessels...
 *
 * PURPOSE:
 * This file provides the single, authoritative definition and memory
 * allocation for the `quicpro_semantic_geometry_config` global variable.
 * =========================================================================
 */
#include "include/config/semantic_geometry/base_layer.h"

/*
 * This is the actual definition and memory allocation for the module's
 * configuration structure.
 */
qp_semantic_geometry_config_t quicpro_semantic_geometry_config;
