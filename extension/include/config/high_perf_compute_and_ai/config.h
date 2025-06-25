/*
 * =========================================================================
 * FILENAME:   include/config/high_perf_compute_and_ai/config.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Hasta la vista, baby.
 *
 * PURPOSE:
 * This header file declares the public C-API for applying runtime
 * configuration changes from PHP userland to the High-Performance
 * Compute & AI module.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_HIGH_PERF_COMPUTE_AI_CONFIG_H
#define QUICPRO_CONFIG_HIGH_PERF_COMPUTE_AI_CONFIG_H

#include "php.h"

/**
 * @brief Applies runtime configuration settings from a PHP array.
 * @param config_arr A zval pointer to a PHP array containing the key-value
 * pairs of the configuration to apply.
 * @return `SUCCESS` if all settings were successfully validated and applied,
 * `FAILURE` otherwise.
 */
int qp_config_high_perf_compute_and_ai_apply_userland_config(zval *config_arr);

#endif /* QUICPRO_CONFIG_HIGH_PERF_COMPUTE_AI_CONFIG_H */
