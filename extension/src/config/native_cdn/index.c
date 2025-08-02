/*
 * =========================================================================
 * FILENAME:   src/config/native_cdn/index.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    What did they look like? Like... sea-bugs.
 *
 * PURPOSE:
 * This file implements the lifecycle functions for the Native CDN
 * configuration module.
 * =========================================================================
 */

#include "include/config/native_cdn/index.h"
#include "include/config/native_cdn/default.h"
#include "include/config/native_cdn/ini.h"
#include "php.h"

/**
 * @brief Initializes the Native CDN configuration module.
 */
void qp_config_native_cdn_init(void)
{
    /* Step 1: Load hardcoded, safe defaults. */
    qp_config_native_cdn_defaults_load();

    /* Step 2: Register INI handlers to override defaults. */
    qp_config_native_cdn_ini_register();
}

/**
 * @brief Shuts down the Native CDN configuration module.
 */
void qp_config_native_cdn_shutdown(void)
{
    qp_config_native_cdn_ini_unregister();
}
