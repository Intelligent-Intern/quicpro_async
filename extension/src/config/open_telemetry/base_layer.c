/*
 * =========================================================================
 * FILENAME:   src/config/open_telemetry/base_layer.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    You know my methods, Watson.
 *
 * PURPOSE:
 * This file provides the single, authoritative definition and memory
 * allocation for the `quicpro_open_telemetry_config` global variable.
 * =========================================================================
 */
#include "include/config/open_telemetry/base_layer.h"

/*
 * This is the actual definition and memory allocation for the module's
 * configuration structure.
 */
qp_open_telemetry_config_t quicpro_open_telemetry_config;
