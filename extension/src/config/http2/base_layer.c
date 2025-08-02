/*
 * =========================================================================
 * FILENAME:   src/config/http2/base_layer.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Last night, Darth Vader came down from Planet Vulcan and told
 * me that if I didn't take Lorraine out, that he'd melt my brain.
 *
 * PURPOSE:
 * This file provides the single, authoritative definition and memory
 * allocation for the `quicpro_http2_config` global variable.
 * =========================================================================
 */
#include "include/config/http2/base_layer.h"

/*
 * This is the actual definition and memory allocation for the module's
 * configuration structure.
 */
qp_http2_config_t quicpro_http2_config;
