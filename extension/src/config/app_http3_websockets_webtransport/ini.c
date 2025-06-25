/*
 * =========================================================================
 * FILENAME:   src/config/app_http3_websockets_webtransport/ini.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    And can you offer me proof of your existence? How can you
 * when neither modern science nor philosophy can explain what life is?
 *
 * PURPOSE:
 * This file implements the registration, parsing, and validation of all
 * `php.ini` settings for the application protocols module.
 * =========================================================================
 */

#include "include/config/app_http3_websockets_webtransport/ini.h"
#include "include/config/app_http3_websockets_webtransport/base_layer.h"

#include "php.h"
#include <zend_ini.h>
#include <ext/spl/spl_exceptions.h>

/*
 * Custom OnUpdate handler for numeric values that must be positive.
 */
static ZEND_INI_MH(OnUpdateAppProtocolPositiveLong)
{
    zend_long val = zend_atol(ZSTR_VAL(new_value), ZSTR_LEN(new_value));

    if (val <= 0) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Invalid value provided for an application protocol directive. A positive integer is required.");
        return FAILURE;
    }

    if (zend_string_equals_literal(entry->name, "quicpro.h3_max_header_list_size")) {
        quicpro_app_protocols_config.h3_max_header_list_size = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.h3_qpack_max_table_capacity")) {
        quicpro_app_protocols_config.h3_qpack_max_table_capacity = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.h3_qpack_blocked_streams")) {
        quicpro_app_protocols_config.h3_qpack_blocked_streams = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.websocket_default_max_payload_size")) {
        quicpro_app_protocols_config.websocket_default_max_payload_size = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.websocket_default_ping_interval_ms")) {
        quicpro_app_protocols_config.websocket_default_ping_interval_ms = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.websocket_handshake_timeout_ms")) {
        quicpro_app_protocols_config.websocket_handshake_timeout_ms = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.webtransport_max_concurrent_sessions")) {
        quicpro_app_protocols_config.webtransport_max_concurrent_sessions = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.webtransport_max_streams_per_session")) {
        quicpro_app_protocols_config.webtransport_max_streams_per_session = val;
    }

    return SUCCESS;
}

/*
 * Custom OnUpdate handler for validating the http_auto_compress string.
 */
static ZEND_INI_MH(OnUpdateCompressionString)
{
    /* This is where a call to a function like qp_validate_comma_separated_string_from_allowlist() would go. */
    /* For now, we just update the string. */
    OnUpdateString(entry, new_value, mh_arg1, mh_arg2, mh_arg3);
    return SUCCESS;
}


PHP_INI_BEGIN()
    /* --- HTTP/3 General Settings --- */
    STD_PHP_INI_ENTRY("quicpro.http_advertise_h3_alt_svc", "1", PHP_INI_SYSTEM, OnUpdateBool, http_advertise_h3_alt_svc, qp_app_protocols_config_t, quicpro_app_protocols_config)
    ZEND_INI_ENTRY_EX("quicpro.http_auto_compress", "brotli,gzip", PHP_INI_SYSTEM, OnUpdateCompressionString, &quicpro_app_protocols_config.http_auto_compress, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.h3_max_header_list_size", "65536", PHP_INI_SYSTEM, OnUpdateAppProtocolPositiveLong, NULL, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.h3_qpack_max_table_capacity", "4096", PHP_INI_SYSTEM, OnUpdateAppProtocolPositiveLong, NULL, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.h3_qpack_blocked_streams", "100", PHP_INI_SYSTEM, OnUpdateAppProtocolPositiveLong, NULL, NULL, NULL)
    STD_PHP_INI_ENTRY("quicpro.h3_server_push_enable", "0", PHP_INI_SYSTEM, OnUpdateBool, h3_server_push_enable, qp_app_protocols_config_t, quicpro_app_protocols_config)
    STD_PHP_INI_ENTRY("quicpro.http_enable_early_hints", "1", PHP_INI_SYSTEM, OnUpdateBool, http_enable_early_hints, qp_app_protocols_config_t, quicpro_app_protocols_config)

    /* --- WebSocket Protocol Settings --- */
    ZEND_INI_ENTRY_EX("quicpro.websocket_default_max_payload_size", "16777216", PHP_INI_SYSTEM, OnUpdateAppProtocolPositiveLong, NULL, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.websocket_default_ping_interval_ms", "25000", PHP_INI_SYSTEM, OnUpdateAppProtocolPositiveLong, NULL, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.websocket_handshake_timeout_ms", "5000", PHP_INI_SYSTEM, OnUpdateAppProtocolPositiveLong, NULL, NULL, NULL)

    /* --- WebTransport Protocol Settings --- */
    STD_PHP_INI_ENTRY("quicpro.webtransport_enable", "1", PHP_INI_SYSTEM, OnUpdateBool, webtransport_enable, qp_app_protocols_config_t, quicpro_app_protocols_config)
    ZEND_INI_ENTRY_EX("quicpro.webtransport_max_concurrent_sessions", "10000", PHP_INI_SYSTEM, OnUpdateAppProtocolPositiveLong, NULL, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.webtransport_max_streams_per_session", "256", PHP_INI_SYSTEM, OnUpdateAppProtocolPositiveLong, NULL, NULL, NULL)
PHP_INI_END()

/**
 * @brief Registers this module's php.ini entries with the Zend Engine.
 */
void qp_config_app_http3_websockets_webtransport_ini_register(void)
{
    REGISTER_INI_ENTRIES();
}

/**
 * @brief Unregisters this module's php.ini entries from the Zend Engine.
 */
void qp_config_app_http3_websockets_webtransport_ini_unregister(void)
{
    UNREGISTER_INI_ENTRIES();
}
