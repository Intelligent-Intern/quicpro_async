/*
 * =========================================================================
 * FILENAME:   src/config/security_and_traffic/ini.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Never gonna say goodbye...
 *
 * PURPOSE:
 * This file implements the registration, parsing, and validation of all
 * `php.ini` settings for the `security_and_traffic` module.
 *
 * ARCHITECTURE:
 * This is a critical file that demonstrates robust INI handling:
 * 1. `OnUpdateSecurityPolicy`: A custom handler that sets the master
 * global security flag `is_userland_override_allowed`.
 * 2. `OnUpdateRateLimiterValue`: A custom handler that performs strict
 * validation on numeric inputs.
 * 3. `OnUpdateCorsOrigins`: A custom handler that uses PHP's internal URL
 * parser for robust validation of the CORS origin string.
 * 4. The `PHP_INI_BEGIN` block uses these custom handlers to enforce security
 * and data integrity at the earliest possible stage.
 * =========================================================================
 */

#include "include/config/security_and_traffic/ini.h"
#include "include/config/security_and_traffic/base_layer.h"
#include "include/quicpro_globals.h" /* CRITICAL: For setting the global flag */

#include "php.h"
#include "main/php_string.h" /* For php_trim */
#include "main/php_url.h"   /* For php_url_parse_ex */
#include <ext/spl/spl_exceptions.h> /* For spl_ce_InvalidArgumentException */
#include <zend_ini.h>

/*
 * This custom OnUpdate handler is the brain of the security policy.
 */
static ZEND_INI_MH(OnUpdateSecurityPolicy)
{
    bool val = zend_ini_parse_bool(new_value);

    if (zend_string_equals_literal(entry->name, "quicpro.security_allow_config_override")) {
        /* This directive exclusively controls the global userland override flag. */
        quicpro_security_config.allow_config_override = val;
        quicpro_globals.is_userland_override_allowed = val;

    } else if (zend_string_equals_literal(entry->name, "quicpro.admin_api_enable")) {
        /* This directive only updates its local config value. */
        /* The Admin API module itself is responsible for checking this flag. */
        quicpro_security_config.admin_api_enable = val;
    }

    return SUCCESS;
}

/*
 * This custom OnUpdate handler validates numeric rate limiter values.
 */
static ZEND_INI_MH(OnUpdateRateLimiterValue)
{
    zend_long val = zend_atol(ZSTR_VAL(new_value), ZSTR_LEN(new_value));

    if (val < 0) {
        zend_throw_exception_ex(
            spl_ce_InvalidArgumentException,
            0,
            "Invalid value provided for a rate-limiter directive. A non-negative integer is required."
        );
        return FAILURE;
    }

    if (zend_string_equals_literal(entry->name, "quicpro.security_rate_limiter_requests_per_sec")) {
        quicpro_security_config.rate_limiter_requests_per_sec = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.security_rate_limiter_burst")) {
        quicpro_security_config.rate_limiter_burst = val;
    }

    return SUCCESS;
}

/*
 * This custom OnUpdate handler uses PHP's internal URL parser for robust
 * validation of the CORS origins string.
 */
static ZEND_INI_MH(OnUpdateCorsOrigins)
{
    char *input_str = ZSTR_VAL(new_value);
    char *input_copy;
    char *token;
    char *saveptr;
    bool is_valid = true;

    /* A single wildcard is always valid. */
    if (strcmp(input_str, "*") == 0) {
        is_valid = true;
    } else {
        /* For anything else, validate it's a comma-separated list of valid origins. */
        input_copy = estrdup(input_str);

        for (token = php_strtok_r(input_copy, ",", &saveptr);
             token != NULL;
             token = php_strtok_r(NULL, ",", &saveptr))
        {
            zend_string *trimmed_token_str;
            php_url *parsed_url;

            trimmed_token_str = php_trim(zend_string_init(token, strlen(token), 0), " \t\n\r\v\f", 7, 3);

            if (ZSTR_LEN(trimmed_token_str) == 0) {
                zend_string_release(trimmed_token_str);
                continue; /* Allow empty entries like "a,,b" */
            }

            parsed_url = php_url_parse_ex(ZSTR_VAL(trimmed_token_str), ZSTR_LEN(trimmed_token_str));

            /* php_url_parse_ex returns NULL on gross parsing errors */
            if (parsed_url == NULL) {
                is_valid = false;
            }
            /* A valid origin MUST have a scheme and a host. */
            else if (parsed_url->scheme == NULL || parsed_url->host == NULL) {
                is_valid = false;
            }
            /* The scheme must be http or https */
            else if (strcasecmp(parsed_url->scheme, "http") != 0 && strcasecmp(parsed_url->scheme, "https") != 0) {
                is_valid = false;
            }

            if (parsed_url) {
                php_url_free(parsed_url);
            }
            zend_string_release(trimmed_token_str);

            if (!is_valid) {
                break;
            }
        }
        efree(input_copy);
    }

    if (!is_valid) {
        zend_throw_exception_ex(
            spl_ce_InvalidArgumentException,
            0,
            "Invalid value provided for CORS origin policy. Value must be '*' or a comma-separated list of valid origins (e.g., 'https://example.com:8443')."
        );
        return FAILURE;
    }

    /* Manually update the string, replicating OnUpdateString's memory handling. */
    if (quicpro_security_config.cors_allowed_origins &&
        (quicpro_security_config.cors_allowed_origins != ZSTR_VAL(entry->def_value))) {
        pefree(quicpro_security_config.cors_allowed_origins, 1);
    }
    quicpro_security_config.cors_allowed_origins = pestrdup(ZSTR_VAL(new_value), 1);

    return SUCCESS;
}


/*
 * Here we define all php.ini entries this module is responsible for.
 */
PHP_INI_BEGIN()
    /* --- Policy Control (Uses custom handler for global state) --- */
    ZEND_INI_ENTRY_EX("quicpro.security_allow_config_override", "0", PHP_INI_SYSTEM, OnUpdateSecurityPolicy, NULL, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.admin_api_enable", "0", PHP_INI_SYSTEM, OnUpdateSecurityPolicy, NULL, NULL, NULL)

    /* --- Rate Limiter (Booleans use standard handler, numerics use custom validation) --- */
    STD_PHP_INI_ENTRY("quicpro.security_rate_limiter_enable", "1", PHP_INI_SYSTEM, OnUpdateBool, rate_limiter_enable, qp_security_config_t, quicpro_security_config)
    ZEND_INI_ENTRY_EX("quicpro.security_rate_limiter_requests_per_sec", "100", PHP_INI_SYSTEM, OnUpdateRateLimiterValue, NULL, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.security_rate_limiter_burst", "50", PHP_INI_SYSTEM, OnUpdateRateLimiterValue, NULL, NULL, NULL)

    /* --- CORS (Uses a custom, robust validation handler) --- */
    ZEND_INI_ENTRY_EX("quicpro.security_cors_allowed_origins", "*", PHP_INI_SYSTEM, OnUpdateCorsOrigins, NULL, NULL, NULL)
PHP_INI_END()

/**
 * @brief Registers this module's php.ini entries with the Zend Engine.
 */
void qp_config_security_ini_register(void)
{
    REGISTER_INI_ENTRIES();
}

/**
 * @brief Unregisters this module's php.ini entries from the Zend Engine.
 */
void qp_config_security_ini_unregister(void)
{
    UNREGISTER_INI_ENTRIES();
}
