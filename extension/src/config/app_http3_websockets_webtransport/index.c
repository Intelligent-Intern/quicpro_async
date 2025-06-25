/*
 * =========================================================================
 * FILENAME:   src/config/app_http3_websockets_webtransport/index.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Section 9: What is it? A ghost? A soul?
 *
 * PURPOSE:
 * This file implements the lifecycle functions for the application
 * protocols configuration module.
 * =========================================================================
 */

#include "include/config/app_http3_websockets_webtransport/index.h"
#include "include/config/app_http3_websockets_webtransport/default.h"
#include "include/config/app_http3_websockets_webtransport/ini.h"
#include "php.h"

/**
 * @brief Initializes the Application Protocols configuration module.
 */
void qp_config_app_http3_websockets_webtransport_init(void)
{
    /* Step 1: Load hardcoded, safe defaults. */
    qp_config_app_http3_websockets_webtransport_defaults_load();

    /* Step 2: Register INI handlers to override defaults. */
    qp_config_app_http3_websockets_webtransport_ini_register();
}

/**
 * @brief Shuts down the Application Protocols configuration module.
 */
void qp_config_app_http3_websockets_webtransport_shutdown(void)
{
    qp_config_app_http3_websockets_webtransport_ini_unregister();
}
