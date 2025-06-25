/*
 * =========================================================================
 * FILENAME:   include/config/open_telemetry/index.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    The game is afoot.
 *
 * PURPOSE:
 * This header file declares the public C-API for the OpenTelemetry
 * configuration module.
 *
 * ARCHITECTURE:
 * It provides the function prototypes for the module's lifecycle, which
 * are called by the master dispatcher to orchestrate loading of settings.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_OPEN_TELEMETRY_INDEX_H
#define QUICPRO_CONFIG_OPEN_TELEMETRY_INDEX_H

/**
 * @brief Initializes the OpenTelemetry configuration module.
 */
void qp_config_open_telemetry_init(void);

/**
 * @brief Shuts down the OpenTelemetry configuration module.
 */
void qp_config_open_telemetry_shutdown(void);

#endif /* QUICPRO_CONFIG_OPEN_TELEMETRY_INDEX_H */
