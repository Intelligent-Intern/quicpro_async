/*
 * =========================================================================
 * FILENAME:   src/config/tcp_transport/config.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    "There is no patch for stupidity, but there is for insecure defaults." – Anonymous OPS
 *
 * PURPOSE:
 *   Implements the logic that applies *runtime* overrides coming from
 *   PHP-userland (either via a Quicpro\Config object or the Admin-API)
 *   to the TCP-Transport configuration structure, after validating every
 *   single parameter.
 * =========================================================================
 */

#include "include/config/tcp_transport/config.h"
#include "include/config/tcp_transport/base_layer.h"
#include "include/quicpro_globals.h"

/* centralised param validators */
#include "include/validation/config_param/validate_bool.h"
#include "include/validation/config_param/validate_positive_long.h"
#include "include/validation/config_param/validate_string_from_allowlist.h"

#include "php.h"
#include <ext/spl/spl_exceptions.h>

int qp_config_tcp_transport_apply_userland_config(zval *config_arr)
{
    /* Step 1: check policy */
    if (!quicpro_globals.is_userland_override_allowed) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0,
            "Configuration override from userland is disabled by system administrator.");
        return FAILURE;
    }

    /* Step 2: must be array */
    if (Z_TYPE_P(config_arr) != IS_ARRAY) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0,
            "Configuration must be provided as an array.");
        return FAILURE;
    }

    /* Step 3: iterate */
    zval *value;
    zend_string *key;

    ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(config_arr), key, value) {
        if (!key) continue;

        /* Boolean params */
        if (zend_string_equals_literal(key, "tcp_enable")) {
            if (qp_validate_bool(value, "tcp_enable") == SUCCESS) {
                quicpro_tcp_transport_config.tcp_enable = zend_is_true(value);
            } else { return FAILURE; }

        } else if (zend_string_equals_literal(key, "tcp_reuse_port_enable")) {
            if (qp_validate_bool(value, "tcp_reuse_port_enable") == SUCCESS) {
                quicpro_tcp_transport_config.tcp_reuse_port_enable = zend_is_true(value);
            } else { return FAILURE; }

        } else if (zend_string_equals_literal(key, "tcp_nodelay_enable")) {
            if (qp_validate_bool(value, "tcp_nodelay_enable") == SUCCESS) {
                quicpro_tcp_transport_config.tcp_nodelay_enable = zend_is_true(value);
            } else { return FAILURE; }

        } else if (zend_string_equals_literal(key, "tcp_cork_enable")) {
            if (qp_validate_bool(value, "tcp_cork_enable") == SUCCESS) {
                quicpro_tcp_transport_config.tcp_cork_enable = zend_is_true(value);
            } else { return FAILURE; }

        } else if (zend_string_equals_literal(key, "tcp_keepalive_enable")) {
            if (qp_validate_bool(value, "tcp_keepalive_enable") == SUCCESS) {
                quicpro_tcp_transport_config.tcp_keepalive_enable = zend_is_true(value);
            } else { return FAILURE; }

        /* Positive longs */
        } else if (zend_string_equals_literal(key, "tcp_max_connections")) {
            if (qp_validate_positive_long(value, &quicpro_tcp_transport_config.tcp_max_connections)
                    != SUCCESS) return FAILURE;

        } else if (zend_string_equals_literal(key, "tcp_connect_timeout_ms")) {
            if (qp_validate_positive_long(value, &quicpro_tcp_transport_config.tcp_connect_timeout_ms)
                    != SUCCESS) return FAILURE;

        } else if (zend_string_equals_literal(key, "tcp_listen_backlog")) {
            if (qp_validate_positive_long(value, &quicpro_tcp_transport_config.tcp_listen_backlog)
                    != SUCCESS) return FAILURE;

        } else if (zend_string_equals_literal(key, "tcp_keepalive_time_sec")) {
            if (qp_validate_positive_long(value, &quicpro_tcp_transport_config.tcp_keepalive_time_sec)
                    != SUCCESS) return FAILURE;

        } else if (zend_string_equals_literal(key, "tcp_keepalive_interval_sec")) {
            if (qp_validate_positive_long(value, &quicpro_tcp_transport_config.tcp_keepalive_interval_sec)
                    != SUCCESS) return FAILURE;

        } else if (zend_string_equals_literal(key, "tcp_keepalive_probes")) {
            if (qp_validate_positive_long(value, &quicpro_tcp_transport_config.tcp_keepalive_probes)
                    != SUCCESS) return FAILURE;

        /* Allowed string lists */
        } else if (zend_string_equals_literal(key, "tcp_tls_min_version_allowed")) {
            const char *allowed[] = {"TLSv1.2", "TLSv1.3", NULL};
            if (qp_validate_string_from_allowlist(value, allowed,
                    &quicpro_tcp_transport_config.tcp_tls_min_version_allowed) != SUCCESS) {
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "tcp_tls_ciphers_tls12")) {
            /* no validation beyond non-empty string */
            if (Z_TYPE_P(value) != IS_STRING || Z_STRLEN_P(value) == 0) {
                zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0,
                    "tcp_tls_ciphers_tls12 must be a non-empty string");
                return FAILURE;
            }
            quicpro_tcp_transport_config.tcp_tls_ciphers_tls12 = estrdup(Z_STRVAL_P(value));
        }
    } ZEND_HASH_FOREACH_END();

    return SUCCESS;
}