/*
 * =========================================================================
 * FILENAME:   src/config/ssh_over_quic/config.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    "The body cannot live without the mind."
 *
 * PURPOSE:
 *   Applies runtime configuration overrides coming from PHP userland (array)
 *   to the SSH‑over‑QUIC module after validating each value.
 * =========================================================================
 */

#include "include/config/ssh_over_quic/config.h"
#include "include/config/ssh_over_quic/base_layer.h"
#include "include/quicpro_globals.h"

/* Centralised validation helpers */
#include "include/validation/config_param/validate_bool.h"
#include "include/validation/config_param/validate_positive_long.h"
#include "include/validation/config_param/validate_string_from_allowlist.h"

#include "php.h"
#include <ext/spl/spl_exceptions.h>

int qp_config_ssh_over_quic_apply_userland_config(zval *config_arr)
{
    /* Step 1: honour global policy */
    if (!quicpro_globals.is_userland_override_allowed) {
        zend_throw_exception_ex(
            spl_ce_InvalidArgumentException, 0,
            "Configuration override from userland is disabled by system administrator.");
        return FAILURE;
    }

    /* Step 2: expect array */
    if (Z_TYPE_P(config_arr) != IS_ARRAY) {
        zend_throw_exception_ex(
            spl_ce_InvalidArgumentException, 0,
            "Configuration must be provided as an associative array.");
        return FAILURE;
    }

    zval *value;
    zend_string *key;

    ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(config_arr), key, value) {
        if (!key) { continue; }

        /* Boolean switches */
        if (zend_string_equals_literal(key, "ssh_gateway_enable")) {
            if (qp_validate_bool(value) == SUCCESS) {
                quicpro_ssh_quic_config.gateway_enable = zend_is_true(value);
            } else { return FAILURE; }
        } else if (zend_string_equals_literal(key, "ssh_gateway_log_session_activity")) {
            if (qp_validate_bool(value) == SUCCESS) {
                quicpro_ssh_quic_config.log_session_activity = zend_is_true(value);
            } else { return FAILURE; }

        /* Free strings */
        } else if (zend_string_equals_literal(key, "ssh_gateway_listen_host")) {
            if (Z_TYPE_P(value) == IS_STRING) {
                quicpro_ssh_quic_config.listen_host = estrdup(Z_STRVAL_P(value));
            } else {
                zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0,
                    "ssh_gateway_listen_host must be a string.");
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "ssh_gateway_default_target_host")) {
            if (Z_TYPE_P(value) == IS_STRING) {
                quicpro_ssh_quic_config.default_target_host = estrdup(Z_STRVAL_P(value));
            } else {
                zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0,
                    "ssh_gateway_default_target_host must be a string.");
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "ssh_gateway_mcp_auth_agent_uri")) {
            if (Z_TYPE_P(value) == IS_STRING) {
                quicpro_ssh_quic_config.mcp_auth_agent_uri = estrdup(Z_STRVAL_P(value));
            } else {
                zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0,
                    "ssh_gateway_mcp_auth_agent_uri must be a string.");
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "ssh_gateway_user_profile_agent_uri")) {
            if (Z_TYPE_P(value) == IS_STRING) {
                quicpro_ssh_quic_config.user_profile_agent_uri = estrdup(Z_STRVAL_P(value));
            } else {
                zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0,
                    "ssh_gateway_user_profile_agent_uri must be a string.");
                return FAILURE;
            }

        /* Positive longs */
        } else if (zend_string_equals_literal(key, "ssh_gateway_listen_port")) {
            if (qp_validate_positive_long(value, &quicpro_ssh_quic_config.listen_port) != SUCCESS) {
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "ssh_gateway_default_target_port")) {
            if (qp_validate_positive_long(value, &quicpro_ssh_quic_config.default_target_port) != SUCCESS) {
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "ssh_gateway_target_connect_timeout_ms")) {
            if (qp_validate_positive_long(value, &quicpro_ssh_quic_config.target_connect_timeout_ms) != SUCCESS) {
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "ssh_gateway_idle_timeout_sec")) {
            if (qp_validate_positive_long(value, &quicpro_ssh_quic_config.idle_timeout_sec) != SUCCESS) {
                return FAILURE;
            }

        /* Allow‑listed strings */
        } else if (zend_string_equals_literal(key, "ssh_gateway_auth_mode")) {
            const char *allowed[] = {"mtls", "mcp_token", NULL};
            if (qp_validate_string_from_allowlist(value, allowed, &quicpro_ssh_quic_config.auth_mode) != SUCCESS) {
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "ssh_gateway_target_mapping_mode")) {
            const char *allowed[] = {"static", "user_profile", NULL};
            if (qp_validate_string_from_allowlist(value, allowed, &quicpro_ssh_quic_config.target_mapping_mode) != SUCCESS) {
                return FAILURE;
            }
        }

    } ZEND_HASH_FOREACH_END();

    return SUCCESS;
}
