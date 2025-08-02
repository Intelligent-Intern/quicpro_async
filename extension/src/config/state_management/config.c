/*
 * =========================================================================
 * FILENAME:   src/config/state_management/config.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    "To define is to limit." — Oscar Wilde
 *
 * PURPOSE:
 *   Implements the logic that validates and applies runtime configuration
 *   overrides provided from PHP userland for the State-Management module.
 * =========================================================================
 */

#include "include/config/state_management/config.h"
#include "include/config/state_management/base_layer.h"
#include "include/quicpro_globals.h"

/* Centralised validation helpers */
#include "include/validation/config_param/validate_string_from_allowlist.h"
#include "include/validation/config_param/validate_string.h"
#include "include/validation/config_param/validate_bool.h"

#include "php.h"
#include <ext/spl/spl_exceptions.h>

int qp_config_state_management_apply_userland_config(zval *config_arr)
{
    zval *value;
    zend_string *key;

    /* 1. Enforce global security policy */
    if (!quicpro_globals.is_userland_override_allowed) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0,
            "Configuration override from userland is disabled by system administrator.");
        return FAILURE;
    }

    /* 2. Ensure we got an array */
    if (Z_TYPE_P(config_arr) != IS_ARRAY) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0,
            "Configuration must be provided as an array.");
        return FAILURE;
    }

    /* 3. Iterate & validate */
    ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(config_arr), key, value) {
        if (!key) {
            continue;
        }
        if (zend_string_equals_literal(key, "state_manager_default_backend")) {
            const char *allowed[] = {"memory", "sqlite", "redis", "postgres", NULL};
            if (qp_validate_string_from_allowlist(value, allowed,
                    &quicpro_state_mgmt_config.default_backend) != SUCCESS) {
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "state_manager_default_uri")) {
            if (qp_validate_string(value, &quicpro_state_mgmt_config.default_uri) != SUCCESS) {
                return FAILURE;
            }
        }
    } ZEND_HASH_FOREACH_END();

    return SUCCESS;
}
