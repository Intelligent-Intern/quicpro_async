/*
 * =========================================================================
 * FILENAME:   src/config/bare_metal_tuning/index.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Quite an experience to live in fear, isn't it? That's what
 * it is to be a slave.
 *
 * PURPOSE:
 * This file implements the lifecycle functions for the bare metal tuning
 * configuration module.
 * =========================================================================
 */

#include "include/config/bare_metal_tuning/index.h"
#include "include/config/bare_metal_tuning/default.h"
#include "include/config/bare_metal_tuning/ini.h"
#include "php.h"

/**
 * @brief Initializes the Bare Metal Tuning configuration module.
 */
void qp_config_bare_metal_tuning_init(void)
{
    /* Step 1: Load hardcoded, safe defaults. */
    qp_config_bare_metal_tuning_defaults_load();

    /* Step 2: Register INI handlers to override defaults. */
    qp_config_bare_metal_tuning_ini_register();
}

/**
 * @brief Shuts down the Bare Metal Tuning configuration module.
 */
void qp_config_bare_metal_tuning_shutdown(void)
{
    qp_config_bare_metal_tuning_ini_unregister();
}
