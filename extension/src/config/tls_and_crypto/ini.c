/*
 * =========================================================================
 * FILENAME:   src/config/tls_and_crypto/ini.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    "Bad defaults are the silent killers of security." – Every CISO
 *
 * PURPOSE:
 *   Registers, parses and validates all php.ini directives for the
 *   TLS & Crypto configuration module.
 * =========================================================================
 */

#include "include/config/tls_and_crypto/ini.h"
#include "include/config/tls_and_crypto/base_layer.h"

#include "php.h"
#include <zend_ini.h>
#include <ext/spl/spl_exceptions.h>

/* --- Utility handlers -------------------------------------------------- */
static ZEND_INI_MH(OnUpdateBool)      { return OnUpdateBool(entry, new_value, mh_arg1, mh_arg2, mh_arg3); }
static ZEND_INI_MH(OnUpdateStringCopy){ return OnUpdateString(entry, new_value, mh_arg1, mh_arg2, mh_arg3); }

static ZEND_INI_MH(OnUpdatePositiveLong)
{
    zend_long v = zend_atol(ZSTR_VAL(new_value), ZSTR_LEN(new_value));
    if (v <= 0) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0,
            "A positive integer greater than zero is required for this directive.");
        return FAILURE;
    }

    if      (zend_string_equals_literal(entry->name, "quicpro.tls_verify_depth"))                  quicpro_tls_crypto_config.tls_verify_depth = v;
    else if (zend_string_equals_literal(entry->name, "quicpro.tls_session_ticket_lifetime_sec"))   quicpro_tls_crypto_config.tls_session_ticket_lifetime_sec = v;
    else if (zend_string_equals_literal(entry->name, "quicpro.tls_server_0rtt_cache_size"))        quicpro_tls_crypto_config.tls_server_0rtt_cache_size = v;
    return SUCCESS;
}

/* --- Directive table --------------------------------------------------- */
PHP_INI_BEGIN()
    /* Transport layer security */
    STD_PHP_INI_ENTRY("quicpro.tls_verify_peer", "1", PHP_INI_SYSTEM, OnUpdateBool,
        tls_verify_peer, qp_tls_crypto_config_t, quicpro_tls_crypto_config)
    ZEND_INI_ENTRY_EX("quicpro.tls_verify_depth", "10", PHP_INI_SYSTEM, OnUpdatePositiveLong, NULL, NULL, NULL)

    ZEND_INI_ENTRY_EX("quicpro.tls_default_ca_file", "", PHP_INI_SYSTEM, OnUpdateStringCopy, &quicpro_tls_crypto_config.tls_default_ca_file, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.tls_default_cert_file", "", PHP_INI_SYSTEM, OnUpdateStringCopy, &quicpro_tls_crypto_config.tls_default_cert_file, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.tls_default_key_file", "", PHP_INI_SYSTEM, OnUpdateStringCopy, &quicpro_tls_crypto_config.tls_default_key_file, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.tls_ticket_key_file", "", PHP_INI_SYSTEM, OnUpdateStringCopy, &quicpro_tls_crypto_config.tls_ticket_key_file, NULL, NULL)

    ZEND_INI_ENTRY_EX("quicpro.tls_ciphers_tls13", "TLS_AES_128_GCM_SHA256:TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256", PHP_INI_SYSTEM, OnUpdateStringCopy, &quicpro_tls_crypto_config.tls_ciphers_tls13, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.tls_curves", "P-256:X25519", PHP_INI_SYSTEM, OnUpdateStringCopy, &quicpro_tls_crypto_config.tls_curves, NULL, NULL)

    ZEND_INI_ENTRY_EX("quicpro.tls_session_ticket_lifetime_sec", "7200", PHP_INI_SYSTEM, OnUpdatePositiveLong, NULL, NULL, NULL)
    STD_PHP_INI_ENTRY("quicpro.tls_enable_early_data", "0", PHP_INI_SYSTEM, OnUpdateBool,
        tls_enable_early_data, qp_tls_crypto_config_t, quicpro_tls_crypto_config)
    ZEND_INI_ENTRY_EX("quicpro.tls_server_0rtt_cache_size", "100000", PHP_INI_SYSTEM, OnUpdatePositiveLong, NULL, NULL, NULL)
    STD_PHP_INI_ENTRY("quicpro.tls_enable_ocsp_stapling", "1", PHP_INI_SYSTEM, OnUpdateBool,
        tls_enable_ocsp_stapling, qp_tls_crypto_config_t, quicpro_tls_crypto_config)

    /* Expert level options */
    STD_PHP_INI_ENTRY("quicpro.tls_enable_ech", "0", PHP_INI_SYSTEM, OnUpdateBool,
        tls_enable_ech, qp_tls_crypto_config_t, quicpro_tls_crypto_config)
    STD_PHP_INI_ENTRY("quicpro.tls_require_ct_policy", "0", PHP_INI_SYSTEM, OnUpdateBool,
        tls_require_ct_policy, qp_tls_crypto_config_t, quicpro_tls_crypto_config)
    STD_PHP_INI_ENTRY("quicpro.tls_disable_sni_validation", "0", PHP_INI_SYSTEM, OnUpdateBool,
        tls_disable_sni_validation, qp_tls_crypto_config_t, quicpro_tls_crypto_config)
    STD_PHP_INI_ENTRY("quicpro.transport_disable_encryption", "0", PHP_INI_SYSTEM, OnUpdateBool,
        transport_disable_encryption, qp_tls_crypto_config_t, quicpro_tls_crypto_config)
PHP_INI_END()

/* --- Register / Unregister -------------------------------------------- */
void qp_config_tls_crypto_ini_register(void)
{
    REGISTER_INI_ENTRIES();
}

void qp_config_tls_crypto_ini_unregister(void)
{
    UNREGISTER_INI_ENTRIES();
}
