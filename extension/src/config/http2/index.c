/*
 * =========================================================================
 * FILENAME:   src/config/http2/index.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    This is heavy.
 *
 * PURPOSE:
 * This file implements the lifecycle functions for the HTTP/2
 * configuration module.
 * =========================================================================
 */

#include "include/config/http2/index.h"
#include "include/config/http2/default.h"
#include "include/config/http2/ini.h"
#include "php.h"

/**
 * @brief Initializes the HTTP/2 configuration module.
 */
void qp_config_http2_init(void)
{
    /* Step 1: Load hardcoded, safe defaults. */
    qp_config_http2_defaults_load();

    /* Step 2: Register INI handlers to override defaults. */
    qp_config_http2_ini_register();
}

/**
 * @brief Shuts down the HTTP/2 configuration module.
 */
void qp_config_http2_shutdown(void)
{
    qp_config_http2_ini_unregister();
}
