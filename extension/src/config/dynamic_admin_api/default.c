/*
 * =========================================================================
 * FILENAME:   src/config/dynamic_admin_api/default.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    I'm a superstitious man. And if some unlucky accident should
 * befall him... then I'm going to blame some of the people in this room.
 *
 * PURPOSE:
 * This file implements the function that loads the hardcoded, default
 * values into the dynamic admin API configuration struct.
 * =========================================================================
 */

#include "include/config/dynamic_admin_api/default.h"
#include "include/config/dynamic_admin_api/base_layer.h"
#include "php.h"

/**
 * @brief Loads the hardcoded, default values into the module's config struct.
 */
void qp_config_dynamic_admin_api_defaults_load(void)
{
    /* Defaults enforce local-only access and strong authentication. */
    quicpro_dynamic_admin_api_config.bind_host = pestrdup("127.0.0.1", 1);
    quicpro_dynamic_admin_api_config.port = 2019;
    quicpro_dynamic_admin_api_config.auth_mode = pestrdup("mtls", 1);
    quicpro_dynamic_admin_api_config.ca_file = pestrdup("", 1);
    quicpro_dynamic_admin_api_config.cert_file = pestrdup("", 1);
    quicpro_dynamic_admin_api_config.key_file = pestrdup("", 1);
}
