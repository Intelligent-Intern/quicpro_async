/*
 * =========================================================================
 * FILENAME:   src/config/dynamic_admin_api/index.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    A man who doesn't spend time with his family can never be
 * a real man.
 *
 * PURPOSE:
 * This file implements the lifecycle functions for the dynamic admin API
 * configuration module.
 * =========================================================================
 */

#include "include/config/dynamic_admin_api/index.h"
#include "include/config/dynamic_admin_api/default.h"
#include "include/config/dynamic_admin_api/ini.h"
#include "php.h"

/**
 * @brief Initializes the Dynamic Admin API configuration module.
 */
void qp_config_dynamic_admin_api_init(void)
{
    /* Step 1: Load hardcoded, safe defaults. */
    qp_config_dynamic_admin_api_defaults_load();

    /* Step 2: Register INI handlers to override defaults. */
    qp_config_dynamic_admin_api_ini_register();
}

/**
 * @brief Shuts down the Dynamic Admin API configuration module.
 */
void qp_config_dynamic_admin_api_shutdown(void)
{
    qp_config_dynamic_admin_api_ini_unregister();
}
