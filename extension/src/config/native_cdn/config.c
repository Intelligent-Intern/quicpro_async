/*
 * =========================================================================
 * FILENAME:   src/config/native_cdn/config.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    You're messing with the wrong guy. I'm the guy that does his
 * job. You must be the other guy.
 *
 * PURPOSE:
 * This file implements the logic for applying runtime configuration
 * changes to the Native CDN module.
 * =========================================================================
 */

#include "include/config/native_cdn/config.h"
#include "include/config/native_cdn/base_layer.h"
#include "include/quicpro_globals.h"

/* Centralized validation helpers */
#include "include/validation/config_param/validate_bool.h"
#include "include/validation/config_param/validate_positive_long.h"
#include "include/validation/config_param/validate_string_from_allowlist.h"
#include "include/validation/config_param/validate_writable_file_path.h"
#include "include/validation/config_param/validate_generic_string.h"
#include "include/validation/config_param/validate_comma_separated_string_from_allowlist.h"

#include "php.h"
#include <ext/spl/spl_exceptions.h>

int qp_config_native_cdn_apply_userland_config(zval *config_arr)
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
        if (!key) continue;

        if (zend_string_equals_literal(key, "enable")) {
            if (qp_validate_bool(value) != SUCCESS) return FAILURE;
            quicpro_native_cdn_config.enable = zend_is_true(value);
        } else if (zend_string_equals_literal(key, "cache_mode")) {
            const char *allowed[] = {"memory", "disk", "hybrid", NULL};
            if (qp_validate_string_from_allowlist(value, allowed, &quicpro_native_cdn_config.cache_mode) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "cache_memory_limit_mb")) {
            if (qp_validate_positive_long(value, &quicpro_native_cdn_config.cache_memory_limit_mb) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "cache_disk_path")) {
            /* This path needs to be writable, not just exist */
            if (qp_validate_writable_file_path(value, &quicpro_native_cdn_config.cache_disk_path) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "cache_default_ttl_sec")) {
            if (qp_validate_positive_long(value, &quicpro_native_cdn_config.cache_default_ttl_sec) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "cache_max_object_size_mb")) {
            if (qp_validate_positive_long(value, &quicpro_native_cdn_config.cache_max_object_size_mb) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "cache_respect_origin_headers")) {
            if (qp_validate_bool(value) != SUCCESS) return FAILURE;
            quicpro_native_cdn_config.cache_respect_origin_headers = zend_is_true(value);
        } else if (zend_string_equals_literal(key, "cache_vary_on_headers")) {
            if (qp_validate_generic_string(value, &quicpro_native_cdn_config.cache_vary_on_headers) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "origin_mcp_endpoint")) {
            if (qp_validate_generic_string(value, &quicpro_native_cdn_config.origin_mcp_endpoint) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "origin_http_endpoint")) {
            if (qp_validate_generic_string(value, &quicpro_native_cdn_config.origin_http_endpoint) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "origin_request_timeout_ms")) {
            if (qp_validate_positive_long(value, &quicpro_native_cdn_config.origin_request_timeout_ms) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "serve_stale_on_error")) {
            if (qp_validate_bool(value) != SUCCESS) return FAILURE;
            quicpro_native_cdn_config.serve_stale_on_error = zend_is_true(value);
        } else if (zend_string_equals_literal(key, "response_headers_to_add")) {
            if (qp_validate_generic_string(value, &quicpro_native_cdn_config.response_headers_to_add) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "allowed_http_methods")) {
            const char *allowed[] = {"GET", "HEAD", "POST", "PUT", "DELETE", "OPTIONS", "PATCH", NULL};
            if (qp_validate_comma_separated_string_from_allowlist(value, allowed, &quicpro_native_cdn_config.allowed_http_methods) != SUCCESS) return FAILURE;
        }

    } ZEND_HASH_FOREACH_END();

    return SUCCESS;
}
