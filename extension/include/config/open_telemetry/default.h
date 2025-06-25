/*
 * =========================================================================
 * FILENAME:   include/config/open_telemetry/default.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    It is a capital mistake to theorize before one has data.
 *
 * PURPOSE:
 * This header declares the function for loading the module's hardcoded
 * default values for the OpenTelemetry integration.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_OPEN_TELEMETRY_DEFAULT_H
#define QUICPRO_CONFIG_OPEN_TELEMETRY_DEFAULT_H

/**
 * @brief Loads the hardcoded, default values into the module's config struct.
 */
void qp_config_open_telemetry_defaults_load(void);

#endif /* QUICPRO_CONFIG_OPEN_TELEMETRY_DEFAULT_H */
