/*
 * =========================================================================
 * FILENAME:   src/config/native_cdn/ini.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Your old man's about to change the world.
 *
 * PURPOSE:
 * This file implements the registration and parsing of all `php.ini`
 * settings for the Native CDN module.
 * =========================================================================
 */

#include "include/config/native_cdn/ini.h"
#include "include/config/native_cdn/base_layer.h"

#include "php.h"
#include <zend_ini.h>
#include <ext/spl/spl_exceptions.h>

/* Custom OnUpdate handler for positive integer values */
static ZEND_INI_MH(OnUpdateCdnPositiveLong)
{
    zend_long val = zend_atol(ZSTR_VAL(new_value), ZSTR_LEN(new_value));
    if (val <= 0) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Invalid value for a CDN directive. A positive integer is required.");
        return FAILURE;
    }

    if (zend_string_equals_literal(entry->name, "quicpro.cdn_cache_memory_limit_mb")) {
        quicpro_native_cdn_config.cache_memory_limit_mb = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.cdn_cache_default_ttl_sec")) {
        quicpro_native_cdn_config.cache_default_ttl_sec = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.cdn_cache_max_object_size_mb")) {
        quicpro_native_cdn_config.cache_max_object_size_mb = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.cdn_origin_request_timeout_ms")) {
        quicpro_native_cdn_config.origin_request_timeout_ms = val;
    }
    return SUCCESS;
}

/* Custom OnUpdate handler for cache_mode string */
static ZEND_INI_MH(OnUpdateCacheMode)
{
    const char *allowed[] = {"memory", "disk", "hybrid", NULL};
    bool is_allowed = false;
    for (int i = 0; allowed[i] != NULL; i++) {
        if (strcasecmp(ZSTR_VAL(new_value), allowed[i]) == 0) {
            is_allowed = true;
            break;
        }
    }
    if (!is_allowed) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Invalid value for CDN cache mode. Must be 'memory', 'disk', or 'hybrid'.");
        return FAILURE;
    }
    OnUpdateString(entry, new_value, mh_arg1, mh_arg2, mh_arg3);
    return SUCCESS;
}

PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("quicpro.cdn_enable", "0", PHP_INI_SYSTEM, OnUpdateBool, enable, qp_native_cdn_config_t, quicpro_native_cdn_config)
    ZEND_INI_ENTRY_EX("quicpro.cdn_cache_mode", "disk", PHP_INI_SYSTEM, OnUpdateCacheMode, &quicpro_native_cdn_config.cache_mode, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.cdn_cache_memory_limit_mb", "512", PHP_INI_SYSTEM, OnUpdateCdnPositiveLong, NULL, NULL, NULL)
    STD_PHP_INI_ENTRY("quicpro.cdn_cache_disk_path", "/var/cache/quicpro_cdn", PHP_INI_SYSTEM, OnUpdateString, cache_disk_path, qp_native_cdn_config_t, quicpro_native_cdn_config)
    ZEND_INI_ENTRY_EX("quicpro.cdn_cache_default_ttl_sec", "86400", PHP_INI_SYSTEM, OnUpdateCdnPositiveLong, NULL, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.cdn_cache_max_object_size_mb", "1024", PHP_INI_SYSTEM, OnUpdateCdnPositiveLong, NULL, NULL, NULL)
    STD_PHP_INI_ENTRY("quicpro.cdn_cache_respect_origin_headers", "1", PHP_INI_SYSTEM, OnUpdateBool, cache_respect_origin_headers, qp_native_cdn_config_t, quicpro_native_cdn_config)
    STD_PHP_INI_ENTRY("quicpro.cdn_cache_vary_on_headers", "Accept-Encoding", PHP_INI_SYSTEM, OnUpdateString, cache_vary_on_headers, qp_native_cdn_config_t, quicpro_native_cdn_config)
    STD_PHP_INI_ENTRY("quicpro.cdn_origin_mcp_endpoint", "", PHP_INI_SYSTEM, OnUpdateString, origin_mcp_endpoint, qp_native_cdn_config_t, quicpro_native_cdn_config)
    STD_PHP_INI_ENTRY("quicpro.cdn_origin_http_endpoint", "", PHP_INI_SYSTEM, OnUpdateString, origin_http_endpoint, qp_native_cdn_config_t, quicpro_native_cdn_config)
    ZEND_INI_ENTRY_EX("quicpro.cdn_origin_request_timeout_ms", "15000", PHP_INI_SYSTEM, OnUpdateCdnPositiveLong, NULL, NULL, NULL)
    STD_PHP_INI_ENTRY("quicpro.cdn_serve_stale_on_error", "1", PHP_INI_SYSTEM, OnUpdateBool, serve_stale_on_error, qp_native_cdn_config_t, quicpro_native_cdn_config)
    STD_PHP_INI_ENTRY("quicpro.cdn_response_headers_to_add", "X-Cache-Status: HIT", PHP_INI_SYSTEM, OnUpdateString, response_headers_to_add, qp_native_cdn_config_t, quicpro_native_cdn_config)
    STD_PHP_INI_ENTRY("quicpro.cdn_allowed_http_methods", "GET,HEAD", PHP_INI_SYSTEM, OnUpdateString, allowed_http_methods, qp_native_cdn_config_t, quicpro_native_cdn_config)
PHP_INI_END()

void qp_config_native_cdn_ini_register(void) { REGISTER_INI_ENTRIES(); }
void qp_config_native_cdn_ini_unregister(void) { UNREGISTER_INI_ENTRIES(); }
