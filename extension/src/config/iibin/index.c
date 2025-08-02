/*
 * =========================================================================
 * FILENAME:   src/config/iibin/index.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    The ships hung in the sky in much the same way that bricks don't.
 *
 * PURPOSE:
 * This file implements the lifecycle functions for the IIBIN serialization
 * configuration module.
 * =========================================================================
 */

#include "include/config/iibin/index.h"
#include "include/config/iibin/default.h"
#include "include/config/iibin/ini.h"
#include "php.h"

/**
 * @brief Initializes the IIBIN Serialization configuration module.
 */
void qp_config_iibin_init(void)
{
    /* Step 1: Load hardcoded, safe defaults. */
    qp_config_iibin_defaults_load();

    /* Step 2: Register INI handlers to override defaults. */
    qp_config_iibin_ini_register();
}

/**
 * @brief Shuts down the IIBIN Serialization configuration module.
 */
void qp_config_iibin_shutdown(void)
{
    qp_config_iibin_ini_unregister();
}
