/*
 * =========================================================================
 * FILENAME:   src/config/security_and_traffic/config.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    ...and hurt you.
 *
 * PURPOSE:
 * This file implements the logic for applying runtime configuration
 * changes to the `security_and_traffic` module.
 *
 * ARCHITECTURE:
 * This implementation is the gatekeeper for runtime changes. It first
 * consults the global security policy. Only if permitted does it proceed
 * to meticulously unpack the user-provided configuration array, passing
 * each value to a dedicated, centralized validation helper before
 * committing it to the internal configuration state. This ensures that no
 * unvalidated data can ever enter the core logic of the extension.
 * =========================================================================
 */

#include "include/config/security_and_traffic/config.h"
#include "include/config/security_and_traffic/base_layer.h"
#include "include/quicpro_globals.h"

/* Centralized validation helpers */
#include "include/validation/config_param/validate_bool.h"
#include "include/validation/config_param/validate_non_negative_long.h"
#include "include/validation/config_param/validate_cors_origin_string.h"

#include "php.h"
#include <ext/spl/spl_exceptions.h> /* For spl_ce_InvalidArgumentException */

int qp_config_security_apply_userland_config(zval *config_arr)
{
    zval *value;
    zend_string *key;

    /*
     * Step 1: Enforce the global security policy. This is the primary
     * security gate for all runtime configuration changes.
     */
    if (!quicpro_globals.is_userland_override_allowed) {
        zend_throw_exception_ex(
            spl_ce_InvalidArgumentException,
            0,
            "Configuration override from userland is disabled by system administrator in php.ini (quicpro.security_allow_config_override)."
        );
        return FAILURE;
    }

    /* Step 2: Ensure we were actually passed an array. */
    if (Z_TYPE_P(config_arr) != IS_ARRAY) {
        zend_throw_exception_ex(
            spl_ce_InvalidArgumentException,
            0,
            "Configuration must be provided as an array."
        );
        return FAILURE;
    }

    /*
     * Step 3: Iterate, validate, and apply each setting.
     */
    ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(config_arr), key, value) {
        if (!key) {
            continue;
        }

        if (zend_string_equals_literal(key, "security_rate_limiter_enable")) {
            if (qp_validate_bool(value) == SUCCESS) {
                quicpro_security_config.rate_limiter_enable = zend_is_true(value);
            } else {
                return FAILURE; /* Exception already thrown by validator */
            }
        } else if (zend_string_equals_literal(key, "security_rate_limiter_requests_per_sec")) {
            if (qp_validate_non_negative_long(value, &quicpro_security_config.rate_limiter_requests_per_sec) != SUCCESS) {
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "security_rate_limiter_burst")) {
            if (qp_validate_non_negative_long(value, &quicpro_security_config.rate_limiter_burst) != SUCCESS) {
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "security_cors_allowed_origins")) {
            char *new_value_str = NULL;
            if (qp_validate_cors_origin_string(value, &new_value_str) == SUCCESS) {
                /* Secure Memory Handling: Free the old string if it exists. */
                if (quicpro_security_config.cors_allowed_origins) {
                    pefree(quicpro_security_config.cors_allowed_origins, 1);
                }
                quicpro_security_config.cors_allowed_origins = new_value_str;
            } else {
                /* On failure, the validator has thrown an exception. */
                return FAILURE;
            }
        }
        /* NOTE: The security policy flags (`allow_config_override` and `admin_api_enable`) */
        /* are intentionally NOT configurable at runtime for security reasons. */

    } ZEND_HASH_FOREACH_END();

    return SUCCESS;
}
