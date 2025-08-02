/*
 * =========================================================================
 * FILENAME:   src/config/open_telemetry/index.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Elementary, my dear Watson.
 *
 * PURPOSE:
 * This file implements the lifecycle functions for the OpenTelemetry
 * configuration module.
 * =========================================================================
 */

#include "include/config/open_telemetry/index.h"
#include "include/config/open_telemetry/default.h"
#include "include/config/open_telemetry/ini.h"
#include "php.h"

/**
 * @brief Initializes the OpenTelemetry configuration module.
 */
void qp_config_open_telemetry_init(void)
{
    /* Step 1: Load hardcoded, safe defaults. */
    qp_config_open_telemetry_defaults_load();

    /* Step 2: Register INI handlers to override defaults. */
    qp_config_open_telemetry_ini_register();
}

/**
 * @brief Shuts down the OpenTelemetry configuration module.
 */
void qp_config_open_telemetry_shutdown(void)
{
    qp_config_open_telemetry_ini_unregister();
}
