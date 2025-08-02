/*
 * =========================================================================
 * FILENAME:   src/config/http2/ini.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Why don't you make like a tree and get out of here?
 *
 * PURPOSE:
 * This file implements the registration and parsing of all `php.ini`
 * settings for the HTTP/2 module.
 * =========================================================================
 */

#include "include/config/http2/ini.h"
#include "include/config/http2/base_layer.h"

#include "php.h"
#include <zend_ini.h>
#include <ext/spl/spl_exceptions.h>

/* Custom OnUpdate handler for positive integer values */
static ZEND_INI_MH(OnUpdateHttp2PositiveLong)
{
    zend_long val = zend_atol(ZSTR_VAL(new_value), ZSTR_LEN(new_value));

    if (val <= 0) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Invalid value for an HTTP/2 directive. A positive integer is required.");
        return FAILURE;
    }

    if (zend_string_equals_literal(entry->name, "quicpro.http2_initial_window_size")) {
        quicpro_http2_config.initial_window_size = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.http2_max_concurrent_streams")) {
        quicpro_http2_config.max_concurrent_streams = val;
    }

    return SUCCESS;
}

/* Custom OnUpdate handler for max_frame_size (specific range) */
static ZEND_INI_MH(OnUpdateHttp2MaxFrameSize)
{
    zend_long val = zend_atol(ZSTR_VAL(new_value), ZSTR_LEN(new_value));

    if (val < 16384 || val > 16777215) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Invalid value for HTTP/2 max_frame_size. Must be between 16384 and 16777215.");
        return FAILURE;
    }
    quicpro_http2_config.max_frame_size = val;
    return SUCCESS;
}


PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("quicpro.http2_enable", "1", PHP_INI_SYSTEM, OnUpdateBool, enable, qp_http2_config_t, quicpro_http2_config)
    ZEND_INI_ENTRY_EX("quicpro.http2_initial_window_size", "65535", PHP_INI_SYSTEM, OnUpdateHttp2PositiveLong, NULL, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.http2_max_concurrent_streams", "100", PHP_INI_SYSTEM, OnUpdateHttp2PositiveLong, NULL, NULL, NULL)
    STD_PHP_INI_ENTRY("quicpro.http2_max_header_list_size", "0", PHP_INI_SYSTEM, OnUpdateLong, max_header_list_size, qp_http2_config_t, quicpro_http2_config)
    STD_PHP_INI_ENTRY("quicpro.http2_enable_push", "1", PHP_INI_SYSTEM, OnUpdateBool, enable_push, qp_http2_config_t, quicpro_http2_config)
    ZEND_INI_ENTRY_EX("quicpro.http2_max_frame_size", "16384", PHP_INI_SYSTEM, OnUpdateHttp2MaxFrameSize, NULL, NULL, NULL)
PHP_INI_END()

void qp_config_http2_ini_register(void) { REGISTER_INI_ENTRIES(); }
void qp_config_http2_ini_unregister(void) { UNREGISTER_INI_ENTRIES(); }
