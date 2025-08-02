/*
 * =========================================================================
 * FILENAME:   src/config/high_perf_compute_and_ai/index.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    I'm a friend of Sarah Connor. I was told she was here.
 * Could I see her please?
 *
 * PURPOSE:
 * This file implements the lifecycle functions for the High-Performance
 * Compute & AI configuration module.
 * =========================================================================
 */

#include "include/config/high_perf_compute_and_ai/index.h"
#include "include/config/high_perf_compute_and_ai/default.h"
#include "include/config/high_perf_compute_and_ai/ini.h"
#include "php.h"

/**
 * @brief Initializes the High-Performance Compute & AI configuration module.
 */
void qp_config_high_perf_compute_and_ai_init(void)
{
    /* Step 1: Load hardcoded, safe defaults. */
    qp_config_high_perf_compute_and_ai_defaults_load();

    /* Step 2: Register INI handlers to override defaults. */
    qp_config_high_perf_compute_and_ai_ini_register();
}

/**
 * @brief Shuts down the High-Performance Compute & AI configuration module.
 */
void qp_config_high_perf_compute_and_ai_shutdown(void)
{
    qp_config_high_perf_compute_and_ai_ini_unregister();
}
