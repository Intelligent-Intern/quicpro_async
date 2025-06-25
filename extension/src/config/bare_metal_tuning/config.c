/*
 * =========================================================================
 * FILENAME:   src/config/bare_metal_tuning/config.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    It's too bad she won't live! But then again, who does?
 *
 * PURPOSE:
 * This file implements the logic for applying runtime configuration
 * changes to the bare metal tuning module.
 * =========================================================================
 */

#include "include/config/bare_metal_tuning/config.h"
#include "include/config/bare_metal_tuning/base_layer.h"
#include "include/quicpro_globals.h"

/* Centralized validation helpers */
#include "include/validation/config_param/validate_bool.h"
#include "include/validation/config_param/validate_non_negative_long.h"
#include "include/validation/config_param/validate_positive_long.h"
#include "include/validation/config_param/validate_string_from_allowlist.h"
#include "include/validation/config_param/validate_generic_string.h"

#include "php.h"
#include <ext/spl/spl_exceptions.h>

int qp_config_bare_metal_tuning_apply_userland_config(zval *config_arr)
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
        if (!key) {
            continue;
        }

        if (zend_string_equals_literal(key, "io_engine_use_uring")) {
            if (qp_validate_bool(value) != SUCCESS) { return FAILURE; }
            quicpro_bare_metal_config.io_engine_use_uring = zend_is_true(value);
        } else if (zend_string_equals_literal(key, "io_uring_sq_poll_ms")) {
            if (qp_validate_non_negative_long(value, &quicpro_bare_metal_config.io_uring_sq_poll_ms) != SUCCESS) {
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "io_max_batch_read_packets")) {
            if (qp_validate_positive_long(value, &quicpro_bare_metal_config.io_max_batch_read_packets) != SUCCESS) {
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "io_max_batch_write_packets")) {
            if (qp_validate_positive_long(value, &quicpro_bare_metal_config.io_max_batch_write_packets) != SUCCESS) {
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "socket_receive_buffer_size")) {
            if (qp_validate_positive_long(value, &quicpro_bare_metal_config.socket_receive_buffer_size) != SUCCESS) {
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "socket_send_buffer_size")) {
            if (qp_validate_positive_long(value, &quicpro_bare_metal_config.socket_send_buffer_size) != SUCCESS) {
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "socket_enable_busy_poll_us")) {
            if (qp_validate_non_negative_long(value, &quicpro_bare_metal_config.socket_enable_busy_poll_us) != SUCCESS) {
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "socket_enable_timestamping")) {
            if (qp_validate_bool(value) != SUCCESS) { return FAILURE; }
            quicpro_bare_metal_config.socket_enable_timestamping = zend_is_true(value);
        } else if (zend_string_equals_literal(key, "io_thread_cpu_affinity")) {
            if (qp_validate_generic_string(value, &quicpro_bare_metal_config.io_thread_cpu_affinity) != SUCCESS) {
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "io_thread_numa_node_policy")) {
            const char *allowed[] = {"default", "prefer", "bind", "interleave", NULL};
            if (qp_validate_string_from_allowlist(value, allowed, &quicpro_bare_metal_config.io_thread_numa_node_policy) != SUCCESS) {
                return FAILURE;
            }
        }

    } ZEND_HASH_FOREACH_END();

    return SUCCESS;
}
