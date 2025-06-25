/*
 * =========================================================================
 * FILENAME:   src/config/app_http3_websockets_webtransport/config.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    I am a living, thinking entity that was created in the
 * sea of information.
 *
 * PURPOSE:
 * This file implements the logic for applying runtime configuration
 * changes to the application protocols module.
 * =========================================================================
 */

#include "include/config/app_http3_websockets_webtransport/config.h"
#include "include/config/app_http3_websockets_webtransport/base_layer.h"
#include "include/quicpro_globals.h"

/* Centralized validation helpers */
#include "include/validation/config_param/validate_bool.h"
#include "include/validation/config_param/validate_positive_long.h"
#include "include/validation/config_param/validate_comma_separated_string_from_allowlist.h"

#include "php.h"
#include <ext/spl/spl_exceptions.h>

int qp_config_app_http3_websockets_webtransport_apply_userland_config(zval *config_arr)
{
    zval *value;
    zend_string *key;

    /* Step 1: Enforce the global security policy. */
    if (!quicpro_globals.is_userland_override_allowed) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Configuration override from userland is disabled by system administrator.");
        return FAILURE;
    }

    /* Step 2: Ensure we have an array. */
    if (Z_TYPE_P(config_arr) != IS_ARRAY) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Configuration must be provided as an array.");
        return FAILURE;
    }

    /* Step 3: Iterate, validate, and apply each setting. */
    ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(config_arr), key, value) {
        if (!key) {
            continue;
        }

        if (zend_string_equals_literal(key, "http_advertise_h3_alt_svc")) {
            if (qp_validate_bool(value) == SUCCESS) {
                quicpro_app_protocols_config.http_advertise_h3_alt_svc = zend_is_true(value);
            } else { return FAILURE; }
        } else if (zend_string_equals_literal(key, "http_auto_compress")) {
            const char *allowed[] = {"brotli", "gzip", "none", NULL};
            if (qp_validate_comma_separated_string_from_allowlist(value, allowed, &quicpro_app_protocols_config.http_auto_compress) != SUCCESS) {
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "h3_max_header_list_size")) {
            if (qp_validate_positive_long(value, &quicpro_app_protocols_config.h3_max_header_list_size) != SUCCESS) {
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "h3_qpack_max_table_capacity")) {
            if (qp_validate_positive_long(value, &quicpro_app_protocols_config.h3_qpack_max_table_capacity) != SUCCESS) {
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "h3_qpack_blocked_streams")) {
            if (qp_validate_positive_long(value, &quicpro_app_protocols_config.h3_qpack_blocked_streams) != SUCCESS) {
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "h3_server_push_enable")) {
            if (qp_validate_bool(value) == SUCCESS) {
                quicpro_app_protocols_config.h3_server_push_enable = zend_is_true(value);
            } else { return FAILURE; }
        } else if (zend_string_equals_literal(key, "http_enable_early_hints")) {
            if (qp_validate_bool(value) == SUCCESS) {
                quicpro_app_protocols_config.http_enable_early_hints = zend_is_true(value);
            } else { return FAILURE; }
        } else if (zend_string_equals_literal(key, "websocket_default_max_payload_size")) {
            if (qp_validate_positive_long(value, &quicpro_app_protocols_config.websocket_default_max_payload_size) != SUCCESS) {
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "websocket_default_ping_interval_ms")) {
            if (qp_validate_positive_long(value, &quicpro_app_protocols_config.websocket_default_ping_interval_ms) != SUCCESS) {
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "websocket_handshake_timeout_ms")) {
            if (qp_validate_positive_long(value, &quicpro_app_protocols_config.websocket_handshake_timeout_ms) != SUCCESS) {
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "webtransport_enable")) {
            if (qp_validate_bool(value) == SUCCESS) {
                quicpro_app_protocols_config.webtransport_enable = zend_is_true(value);
            } else { return FAILURE; }
        } else if (zend_string_equals_literal(key, "webtransport_max_concurrent_sessions")) {
            if (qp_validate_positive_long(value, &quicpro_app_protocols_config.webtransport_max_concurrent_sessions) != SUCCESS) {
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "webtransport_max_streams_per_session")) {
            if (qp_validate_positive_long(value, &quicpro_app_protocols_config.webtransport_max_streams_per_session) != SUCCESS) {
                return FAILURE;
            }
        }

    } ZEND_HASH_FOREACH_END();

    return SUCCESS;
}
