/*
 * =========================================================================
 * FILENAME:   src/config/native_object_store/config.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    You shall not pass!
 *
 * PURPOSE:
 * This file implements the logic for applying runtime configuration
 * changes to the Native Object Store module.
 * =========================================================================
 */

#include "include/config/native_object_store/config.h"
#include "include/config/native_object_store/base_layer.h"
#include "include/quicpro_globals.h"

/* Centralized validation helpers */
#include "include/validation/config_param/validate_bool.h"
#include "include/validation/config_param/validate_positive_long.h"
#include "include/validation/config_param/validate_string_from_allowlist.h"
#include "include/validation/config_param/validate_generic_string.h"
#include "include/validation/config_param/validate_erasure_coding_shards_string.h"

#include "php.h"
#include <ext/spl/spl_exceptions.h>

int qp_config_native_object_store_apply_userland_config(zval *config_arr)
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
            quicpro_native_object_store_config.enable = zend_is_true(value);
        } else if (zend_string_equals_literal(key, "s3_api_compat_enable")) {
            if (qp_validate_bool(value) != SUCCESS) return FAILURE;
            quicpro_native_object_store_config.s3_api_compat_enable = zend_is_true(value);
        } else if (zend_string_equals_literal(key, "versioning_enable")) {
            if (qp_validate_bool(value) != SUCCESS) return FAILURE;
            quicpro_native_object_store_config.versioning_enable = zend_is_true(value);
        } else if (zend_string_equals_literal(key, "allow_anonymous_access")) {
            if (qp_validate_bool(value) != SUCCESS) return FAILURE;
            quicpro_native_object_store_config.allow_anonymous_access = zend_is_true(value);
        } else if (zend_string_equals_literal(key, "default_redundancy_mode")) {
            const char *allowed[] = {"erasure_coding", "replication", NULL};
            if (qp_validate_string_from_allowlist(value, allowed, &quicpro_native_object_store_config.default_redundancy_mode) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "erasure_coding_shards")) {
            if (qp_validate_erasure_coding_shards_string(value, &quicpro_native_object_store_config.erasure_coding_shards) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "default_replication_factor")) {
            if (qp_validate_positive_long(value, &quicpro_native_object_store_config.default_replication_factor) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "default_chunk_size_mb")) {
            if (qp_validate_positive_long(value, &quicpro_native_object_store_config.default_chunk_size_mb) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "metadata_agent_uri")) {
            if (qp_validate_generic_string(value, &quicpro_native_object_store_config.metadata_agent_uri) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "node_discovery_mode")) {
            const char *allowed[] = {"static", "mcp_heartbeat", NULL};
            if (qp_validate_string_from_allowlist(value, allowed, &quicpro_native_object_store_config.node_discovery_mode) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "node_static_list")) {
            if (qp_validate_generic_string(value, &quicpro_native_object_store_config.node_static_list) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "metadata_cache_enable")) {
            if (qp_validate_bool(value) != SUCCESS) return FAILURE;
            quicpro_native_object_store_config.metadata_cache_enable = zend_is_true(value);
        } else if (zend_string_equals_literal(key, "metadata_cache_ttl_sec")) {
            if (qp_validate_positive_long(value, &quicpro_native_object_store_config.metadata_cache_ttl_sec) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "enable_directstorage")) {
            if (qp_validate_bool(value) != SUCCESS) return FAILURE;
            quicpro_native_object_store_config.enable_directstorage = zend_is_true(value);
        }

    } ZEND_HASH_FOREACH_END();

    return SUCCESS;
}
