/*
 * =========================================================================
 * FILENAME:   src/config/high_perf_compute_and_ai/base_layer.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    The system goes on-line August 4th, 1997.
 *
 * PURPOSE:
 * This file provides the single, authoritative definition and memory
 * allocation for the `quicpro_high_perf_compute_ai_config` global variable.
 * =========================================================================
 */
#include "include/config/high_perf_compute_and_ai/base_layer.h"

/*
 * This is the actual definition and memory allocation for the module's
 * configuration structure.
 */
qp_high_perf_compute_ai_config_t quicpro_high_perf_compute_ai_config;
