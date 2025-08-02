/*
 * =========================================================================
 * FILENAME:   src/config/dynamic_admin_api/config.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Never tell anybody outside the family what you're thinking.
 *
 * PURPOSE:
 * This file implements the logic for applying runtime configuration
 * changes to the dynamic admin API module.
 * =========================================================================
 */

#include "include/config/dynamic_admin_api/config.h"
#include "include/config/dynamic_admin_api/base_layer.h"
#include "include/quicpro_globals.h"

/* Centralized validation helpers */
#include "include/validation/config_param/validate_long_range.h"
#include "include/validation/config_param/validate_string_from_allowlist.h"
#include "include/validation/config_param/validate_host_string.h"
#include "include/validation/config_param/validate_readable_file_path.h"


#include "php.h"
#include <ext/spl/spl_exceptions.h>

int qp_config_dynamic_admin_api_apply_userland_config(zval *config_arr)
{
    zval *value;
    zend_string *key;

    if (!quicpro_globals.is_userland_override_allowed) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Configuration override from userland is disabled by system administrator.");
        return FAILURE;
    }

    if (Z_TYPE_P(config_arr) != IS_ARRAY) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Configuration must be provided as an array.");
        return FAILURE;
    }

    ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(config_arr), key, value) {
        if (!key) continue;

        if (zend_string_equals_literal(key, "bind_host")) {
            if (qp_validate_host_string(value, &quicpro_dynamic_admin_api_config.bind_host) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "port")) {
            if (qp_validate_long_range(value, 1024, 65535, &quicpro_dynamic_admin_api_config.port) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "auth_mode")) {
            const char *allowed[] = {"mtls", NULL};
            if (qp_validate_string_from_allowlist(value, allowed, &quicpro_dynamic_admin_api_config.auth_mode) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "ca_file")) {
            if (qp_validate_readable_file_path(value, &quicpro_dynamic_admin_api_config.ca_file) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "cert_file")) {
            if (qp_validate_readable_file_path(value, &quicpro_dynamic_admin_api_config.cert_file) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "key_file")) {
            if (qp_validate_readable_file_path(value, &quicpro_dynamic_admin_api_config.key_file) != SUCCESS) return FAILURE;
        }

    } ZEND_HASH_FOREACH_END();

    return SUCCESS;
}
