/*
 * =========================================================================
 * FILENAME:   src/config/http2/config.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    It's 'get out of town,' idiot.
 *
 * PURPOSE:
 * This file implements the logic for applying runtime configuration
 * changes to the HTTP/2 module.
 * =========================================================================
 */

#include "include/config/http2/config.h"
#include "include/config/http2/base_layer.h"
#include "include/quicpro_globals.h"

/* Centralized validation helpers */
#include "include/validation/config_param/validate_bool.h"
#include "include/validation/config_param/validate_positive_long.h"
#include "include/validation/config_param/validate_non_negative_long.h"
#include "include/validation/config_param/validate_long_range.h"

#include "php.h"
#include <ext/spl/spl_exceptions.h>

int qp_config_http2_apply_userland_config(zval *config_arr)
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

        if (zend_string_equals_literal(key, "enable")) {
            if (qp_validate_bool(value) != SUCCESS) return FAILURE;
            quicpro_http2_config.enable = zend_is_true(value);
        } else if (zend_string_equals_literal(key, "initial_window_size")) {
            if (qp_validate_positive_long(value, &quicpro_http2_config.initial_window_size) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "max_concurrent_streams")) {
            if (qp_validate_positive_long(value, &quicpro_http2_config.max_concurrent_streams) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "max_header_list_size")) {
            if (qp_validate_non_negative_long(value, &quicpro_http2_config.max_header_list_size) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "enable_push")) {
            if (qp_validate_bool(value) != SUCCESS) return FAILURE;
            quicpro_http2_config.enable_push = zend_is_true(value);
        } else if (zend_string_equals_literal(key, "max_frame_size")) {
            if (qp_validate_long_range(value, 16384, 16777215, &quicpro_http2_config.max_frame_size) != SUCCESS) return FAILURE;
        }

    } ZEND_HASH_FOREACH_END();

    return SUCCESS;
}
