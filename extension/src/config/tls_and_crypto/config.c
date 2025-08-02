/*
 * =========================================================================
 * FILENAME:   src/config/tls_and_crypto/config.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    "Trust, but verify – and then double-check the cipher-suite."
 *
 * PURPOSE:
 *   Implements the userland override mechanism for TLS & Crypto options.
 *   Values arrive as a PHP associative array, are strictly validated and
 *   then written atomically into the live config struct.
 * =========================================================================
 */

#include "include/config/tls_and_crypto/config.h"
#include "include/config/tls_and_crypto/base_layer.h"
#include "include/quicpro_globals.h"

#include "include/validation/config_param/validate_bool.h"
#include "include/validation/config_param/validate_positive_long.h"
#include "include/validation/config_param/validate_string.h"

#include "php.h"
#include <ext/spl/spl_exceptions.h>

int qp_config_tls_crypto_apply_userland_config(zval *arr)
{
    if (!quicpro_globals.is_userland_override_allowed) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0,
            "Configuration override from userland is disabled by administrator.");
        return FAILURE;
    }

    if (Z_TYPE_P(arr) != IS_ARRAY) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0,
            "Configuration must be provided as an associative array.");
        return FAILURE;
    }

    zval *val; zend_string *key;
    ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(arr), key, val) {
        if (!key) continue;

        /* --- Booleans --- */
        if (zend_string_equals_literal(key, "tls_verify_peer")) {
            if (qp_validate_bool(val, "tls_verify_peer") == SUCCESS)
                quicpro_tls_crypto_config.tls_verify_peer = zend_is_true(val);
            else return FAILURE;

        } else if (zend_string_equals_literal(key, "tls_enable_early_data")) {
            if (qp_validate_bool(val, "tls_enable_early_data") == SUCCESS)
                quicpro_tls_crypto_config.tls_enable_early_data = zend_is_true(val);
            else return FAILURE;

        } else if (zend_string_equals_literal(key, "tls_enable_ocsp_stapling")) {
            if (qp_validate_bool(val, "tls_enable_ocsp_stapling") == SUCCESS)
                quicpro_tls_crypto_config.tls_enable_ocsp_stapling = zend_is_true(val);
            else return FAILURE;

        } else if (zend_string_equals_literal(key, "tls_enable_ech")) {
            if (qp_validate_bool(val, "tls_enable_ech") == SUCCESS)
                quicpro_tls_crypto_config.tls_enable_ech = zend_is_true(val);
            else return FAILURE;

        } else if (zend_string_equals_literal(key, "tls_require_ct_policy")) {
            if (qp_validate_bool(val, "tls_require_ct_policy") == SUCCESS)
                quicpro_tls_crypto_config.tls_require_ct_policy = zend_is_true(val);
            else return FAILURE;

        } else if (zend_string_equals_literal(key, "tls_disable_sni_validation")) {
            if (qp_validate_bool(val, "tls_disable_sni_validation") == SUCCESS)
                quicpro_tls_crypto_config.tls_disable_sni_validation = zend_is_true(val);
            else return FAILURE;

        } else if (zend_string_equals_literal(key, "transport_disable_encryption")) {
            if (qp_validate_bool(val, "transport_disable_encryption") == SUCCESS)
                quicpro_tls_crypto_config.transport_disable_encryption = zend_is_true(val);
            else return FAILURE;

        /* --- Positive longs --- */
        } else if (zend_string_equals_literal(key, "tls_verify_depth")) {
            if (qp_validate_positive_long(val, &quicpro_tls_crypto_config.tls_verify_depth) != SUCCESS)
                return FAILURE;

        } else if (zend_string_equals_literal(key, "tls_session_ticket_lifetime_sec")) {
            if (qp_validate_positive_long(val, &quicpro_tls_crypto_config.tls_session_ticket_lifetime_sec) != SUCCESS)
                return FAILURE;

        } else if (zend_string_equals_literal(key, "tls_server_0rtt_cache_size")) {
            if (qp_validate_positive_long(val, &quicpro_tls_crypto_config.tls_server_0rtt_cache_size) != SUCCESS)
                return FAILURE;

        /* --- Strings / paths / cipher lists --- */
        } else if (zend_string_equals_literal(key, "tls_default_ca_file")) {
            if (qp_validate_string(val, &quicpro_tls_crypto_config.tls_default_ca_file) != SUCCESS)
                return FAILURE;

        } else if (zend_string_equals_literal(key, "tls_default_cert_file")) {
            if (qp_validate_string(val, &quicpro_tls_crypto_config.tls_default_cert_file) != SUCCESS)
                return FAILURE;

        } else if (zend_string_equals_literal(key, "tls_default_key_file")) {
            if (qp_validate_string(val, &quicpro_tls_crypto_config.tls_default_key_file) != SUCCESS)
                return FAILURE;

        } else if (zend_string_equals_literal(key, "tls_ticket_key_file")) {
            if (qp_validate_string(val, &quicpro_tls_crypto_config.tls_ticket_key_file) != SUCCESS)
                return FAILURE;

        } else if (zend_string_equals_literal(key, "tls_ciphers_tls13")) {
            if (qp_validate_string(val, &quicpro_tls_crypto_config.tls_ciphers_tls13) != SUCCESS)
                return FAILURE;

        } else if (zend_string_equals_literal(key, "tls_curves")) {
            if (qp_validate_string(val, &quicpro_tls_crypto_config.tls_curves) != SUCCESS)
                return FAILURE;
        }
    } ZEND_HASH_FOREACH_END();

    return SUCCESS;
}
