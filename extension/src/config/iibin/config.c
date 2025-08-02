/*
 * =========================================================================
 * FILENAME:   src/config/iibin/config.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Time is an illusion. Lunchtime doubly so.
 *
 * PURPOSE:
 * This file implements the logic for applying runtime configuration
 * changes to the IIBIN serialization module.
 * =========================================================================
 */

#include "include/config/iibin/config.h"
#include "include/config/iibin/base_layer.h"
#include "include/quicpro_globals.h"

/* Centralized validation helpers */
#include "include/validation/config_param/validate_bool.h"
#include "include/validation/config_param/validate_positive_long.h"
#include "include/validation/config_param/validate_generic_string.h"

#include "php.h"
#include <ext/spl/spl_exceptions.h>

int qp_config_iibin_apply_userland_config(zval *config_arr)
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

        if (zend_string_equals_literal(key, "max_schema_fields")) {
            if (qp_validate_positive_long(value, &quicpro_iibin_config.max_schema_fields) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "max_recursion_depth")) {
            if (qp_validate_positive_long(value, &quicpro_iibin_config.max_recursion_depth) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "string_interning_enable")) {
            if (qp_validate_bool(value) != SUCCESS) return FAILURE;
            quicpro_iibin_config.string_interning_enable = zend_is_true(value);
        } else if (zend_string_equals_literal(key, "use_shared_memory_buffers")) {
            if (qp_validate_bool(value) != SUCCESS) return FAILURE;
            quicpro_iibin_config.use_shared_memory_buffers = zend_is_true(value);
        } else if (zend_string_equals_literal(key, "default_buffer_size_kb")) {
            if (qp_validate_positive_long(value, &quicpro_iibin_config.default_buffer_size_kb) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "shm_total_memory_mb")) {
            if (qp_validate_positive_long(value, &quicpro_iibin_config.shm_total_memory_mb) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "shm_path")) {
            if (qp_validate_generic_string(value, &quicpro_iibin_config.shm_path) != SUCCESS) return FAILURE;
        }

    } ZEND_HASH_FOREACH_END();

    return SUCCESS;
}
