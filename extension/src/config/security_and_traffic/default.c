/*
 * =========================================================================
 * FILENAME:   src/config/security_and_traffic/default.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Never gonna run around and desert you...
 *
 * PURPOSE:
 * This file implements the function that loads the hardcoded, secure-
 * by-default values into the `security_and_traffic` configuration struct.
 *
 * ARCHITECTURE:
 * This is the first and most fundamental step in the configuration
 * loading sequence. It establishes a known, safe baseline before any
 * external configuration (like php.ini) is considered. The principle
 * of "secure by default" is enforced here by explicitly disabling
 * runtime overrides and the Admin API until they are consciously enabled
 * by a system administrator in php.ini.
 * =========================================================================
 */

#include "include/config/security_and_traffic/default.h"
#include "include/config/security_and_traffic/base_layer.h"
#include "php.h"

/**
 * @brief Loads the hardcoded, secure-by-default values into the module's
 * config struct. This is the first step in the configuration load sequence.
 */
void qp_config_security_defaults_load(void)
{
    /*
     * These are the conservative, "secure by default" values.
     */

    /* Policy Control: Overrides are forbidden by default. */
    quicpro_security_config.allow_config_override = false;
    quicpro_security_config.admin_api_enable = false;

    /* Rate Limiter: Enabled with conservative limits by default. */
    quicpro_security_config.rate_limiter_enable = true;
    quicpro_security_config.rate_limiter_requests_per_sec = 100;
    quicpro_security_config.rate_limiter_burst = 50;

    /* CORS: A permissive default, as it's a browser-enforced security model. */
    /* A more secure default for APIs might be an empty string "". */
    quicpro_security_config.cors_allowed_origins = pestrdup("*", 1);
}
