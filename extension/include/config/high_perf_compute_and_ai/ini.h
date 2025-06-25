/*
 * =========================================================================
 * FILENAME:   include/config/high_perf_compute_and_ai/ini.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    The future is not set. There is no fate but what we make for ourselves.
 *
 * PURPOSE:
 * This header file declares the internal C-API for handling the `php.ini`
 * entries of the High-Performance Compute & AI configuration module.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_HIGH_PERF_COMPUTE_AI_INI_H
#define QUICPRO_CONFIG_HIGH_PERF_COMPUTE_AI_INI_H

/**
 * @brief Registers this module's php.ini entries with the Zend Engine.
 */
void qp_config_high_perf_compute_and_ai_ini_register(void);

/**
 * @brief Unregisters this module's php.ini entries from the Zend Engine.
 */
void qp_config_high_perf_compute_and_ai_ini_unregister(void);

#endif /* QUICPRO_CONFIG_HIGH_PERF_COMPUTE_AI_INI_H */
