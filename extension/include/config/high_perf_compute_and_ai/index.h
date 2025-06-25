/*
 * =========================================================================
 * FILENAME:   include/config/high_perf_compute_and_ai/index.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Come with me if you want to live.
 *
 * PURPOSE:
 * This header file declares the public C-API for the High-Performance
 * Compute & AI configuration module.
 *
 * ARCHITECTURE:
 * It provides the function prototypes for the module's lifecycle, which
 * are called by the master dispatcher to orchestrate loading of settings.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_HIGH_PERF_COMPUTE_AI_INDEX_H
#define QUICPRO_CONFIG_HIGH_PERF_COMPUTE_AI_INDEX_H

/**
 * @brief Initializes the High-Performance Compute & AI configuration module.
 */
void qp_config_high_perf_compute_and_ai_init(void);

/**
 * @brief Shuts down the High-Performance Compute & AI configuration module.
 */
void qp_config_high_perf_compute_and_ai_shutdown(void);

#endif /* QUICPRO_CONFIG_HIGH_PERF_COMPUTE_AI_INDEX_H */
