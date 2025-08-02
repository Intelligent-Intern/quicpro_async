/*
 * =========================================================================
 * FILENAME:   src/config/native_cdn/base_layer.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    The Grid. A digital frontier. I tried to picture clusters
 * of information as they moved through the computer.
 *
 * PURPOSE:
 * This file provides the single, authoritative definition and memory
 * allocation for the `quicpro_native_cdn_config` global variable.
 * =========================================================================
 */
#include "include/config/native_cdn/base_layer.h"

/*
 * This is the actual definition and memory allocation for the module's
 * configuration structure.
 */
qp_native_cdn_config_t quicpro_native_cdn_config;
