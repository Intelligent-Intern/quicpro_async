/*
 * =========================================================================
 * FILENAME:   src/config/tcp_transport/ini.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    "Turning knobs is fun until you realise they’re live in production." – Random SRE
 *
 * PURPOSE:
 *   Registers, parses and validates *all* php.ini directives that belong to
 *   the TCP‑Transport module.
 * =========================================================================
 */

#include "include/config/tcp_transport/ini.h"
#include "include/config/tcp_transport/base_layer.h"

#include "php.h"
#include <zend_ini.h>
#include <ext/spl/spl_exceptions.h>

/* positive‑long helper shared by many directives */
static ZEND_INI_MH(OnUpdateTcpPositiveLong)
{
    zend_long val = zend_atol(ZSTR_VAL(new_value), ZSTR_LEN(new_value));
    if (val <= 0) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0,
            "A positive integer value is required for this directive.");
        return FAILURE;
    }

    if      (zend_string_equals_literal(entry->name, "quicpro.tcp_max_connections"))           quicpro_tcp_transport_config.tcp_max_connections        = val;
    else if (zend_string_equals_literal(entry->name, "quicpro.tcp_connect_timeout_ms"))        quicpro_tcp_transport_config.tcp_connect_timeout_ms     = val;
    else if (zend_string_equals_literal(entry->name, "quicpro.tcp_listen_backlog"))            quicpro_tcp_transport_config.tcp_listen_backlog         = val;
    else if (zend_string_equals_literal(entry->name, "quicpro.tcp_keepalive_time_sec"))        quicpro_tcp_transport_config.tcp_keepalive_time_sec     = val;
    else if (zend_string_equals_literal(entry->name, "quicpro.tcp_keepalive_interval_sec"))    quicpro_tcp_transport_config.tcp_keepalive_interval_sec = val;
    else if (zend_string_equals_literal(entry->name, "quicpro.tcp_keepalive_probes"))          quicpro_tcp_transport_config.tcp_keepalive_probes       = val;

    return SUCCESS;
}

/* allowlist TLS versions */
static ZEND_INI_MH(OnUpdateTlsMinVersion)
{
    if (!zend_string_equals_literal(new_value, "TLSv1.2") &&
        !zend_string_equals_literal(new_value, "TLSv1.3")) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0,
            "Invalid tcp_tls_min_version_allowed: expected TLSv1.2 or TLSv1.3");
        return FAILURE;
    }
    quicpro_tcp_transport_config.tcp_tls_min_version_allowed = estrdup(ZSTR_VAL(new_value));
    return SUCCESS;
}

/* simple string copy */
static ZEND_INI_MH(OnUpdateString)
{
    OnUpdateString(entry, new_value, mh_arg1, mh_arg2, mh_arg3);
    return SUCCESS;
}

PHP_INI_BEGIN()
    /* Connection management */
    STD_PHP_INI_ENTRY("quicpro.tcp_enable", "1", PHP_INI_SYSTEM, OnUpdateBool,
        tcp_enable, qp_tcp_transport_config_t, quicpro_tcp_transport_config)
    ZEND_INI_ENTRY_EX("quicpro.tcp_max_connections", "10240", PHP_INI_SYSTEM, OnUpdateTcpPositiveLong, NULL, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.tcp_connect_timeout_ms", "5000", PHP_INI_SYSTEM, OnUpdateTcpPositiveLong, NULL, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.tcp_listen_backlog", "511", PHP_INI_SYSTEM, OnUpdateTcpPositiveLong, NULL, NULL, NULL)

    /* Socket options */
    STD_PHP_INI_ENTRY("quicpro.tcp_reuse_port_enable", "1", PHP_INI_SYSTEM, OnUpdateBool,
        tcp_reuse_port_enable, qp_tcp_transport_config_t, quicpro_tcp_transport_config)
    STD_PHP_INI_ENTRY("quicpro.tcp_nodelay_enable", "0", PHP_INI_SYSTEM, OnUpdateBool,
        tcp_nodelay_enable, qp_tcp_transport_config_t, quicpro_tcp_transport_config)
    STD_PHP_INI_ENTRY("quicpro.tcp_cork_enable", "0", PHP_INI_SYSTEM, OnUpdateBool,
        tcp_cork_enable, qp_tcp_transport_config_t, quicpro_tcp_transport_config)

    /* Keep‑Alive */
    STD_PHP_INI_ENTRY("quicpro.tcp_keepalive_enable", "1", PHP_INI_SYSTEM, OnUpdateBool,
        tcp_keepalive_enable, qp_tcp_transport_config_t, quicpro_tcp_transport_config)
    ZEND_INI_ENTRY_EX("quicpro.tcp_keepalive_time_sec", "7200", PHP_INI_SYSTEM, OnUpdateTcpPositiveLong, NULL, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.tcp_keepalive_interval_sec", "75", PHP_INI_SYSTEM, OnUpdateTcpPositiveLong, NULL, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.tcp_keepalive_probes", "9", PHP_INI_SYSTEM, OnUpdateTcpPositiveLong, NULL, NULL, NULL)

    /* TLS over TCP */
    ZEND_INI_ENTRY_EX("quicpro.tcp_tls_min_version_allowed", "TLSv1.2", PHP_INI_SYSTEM, OnUpdateTlsMinVersion, NULL, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.tcp_tls_ciphers_tls12", "ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384", PHP_INI_SYSTEM, OnUpdateString, &quicpro_tcp_transport_config.tcp_tls_ciphers_tls12, NULL, NULL)
PHP_INI_END()

void qp_config_tcp_transport_ini_register(void)
{
    REGISTER_INI_ENTRIES();
}

void qp_config_tcp_transport_ini_unregister(void)
{
    UNREGISTER_INI_ENTRIES();
}
