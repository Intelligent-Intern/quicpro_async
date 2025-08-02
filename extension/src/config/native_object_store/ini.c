/*
 * =========================================================================
 * FILENAME:   src/config/native_object_store/ini.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    I don't know half of you half as well as I should like;
 * and I like less than half of you half as well as you deserve.
 *
 * PURPOSE:
 * This file implements the registration, parsing, and validation of all
 * `php.ini` settings for the Native Object Store module.
 * =========================================================================
 */

#include "include/config/native_object_store/ini.h"
#include "include/config/native_object_store/base_layer.h"

#include "php.h"
#include <zend_ini.h>
#include <ext/spl/spl_exceptions.h>
#include <stdio.h> /* For sscanf in OnUpdateErasureCodingShards */

/* Custom OnUpdate handler for positive integer values */
static ZEND_INI_MH(OnUpdateObjectStorePositiveLong)
{
    zend_long val = zend_atol(ZSTR_VAL(new_value), ZSTR_LEN(new_value));
    if (val <= 0) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Invalid value for an object store directive. A positive integer is required.");
        return FAILURE;
    }

    if (zend_string_equals_literal(entry->name, "quicpro.storage_default_replication_factor")) {
        quicpro_native_object_store_config.default_replication_factor = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.storage_default_chunk_size_mb")) {
        quicpro_native_object_store_config.default_chunk_size_mb = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.storage_metadata_cache_ttl_sec")) {
        quicpro_native_object_store_config.metadata_cache_ttl_sec = val;
    }
    return SUCCESS;
}

/* Custom OnUpdate handler for redundancy_mode string */
static ZEND_INI_MH(OnUpdateRedundancyMode)
{
    const char *allowed[] = {"erasure_coding", "replication", NULL};
    bool is_allowed = false;
    for (int i = 0; allowed[i] != NULL; i++) {
        if (strcasecmp(ZSTR_VAL(new_value), allowed[i]) == 0) {
            is_allowed = true;
            break;
        }
    }
    if (!is_allowed) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Invalid value for redundancy mode. Must be 'erasure_coding' or 'replication'.");
        return FAILURE;
    }
    OnUpdateString(entry, new_value, mh_arg1, mh_arg2, mh_arg3);
    return SUCCESS;
}

/* Custom OnUpdate handler for node_discovery_mode string */
static ZEND_INI_MH(OnUpdateDiscoveryMode)
{
    const char *allowed[] = {"static", "mcp_heartbeat", NULL};
    bool is_allowed = false;
    for (int i = 0; allowed[i] != NULL; i++) {
        if (strcasecmp(ZSTR_VAL(new_value), allowed[i]) == 0) {
            is_allowed = true;
            break;
        }
    }
    if (!is_allowed) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Invalid value for node discovery mode. Must be 'static' or 'mcp_heartbeat'.");
        return FAILURE;
    }
    OnUpdateString(entry, new_value, mh_arg1, mh_arg2, mh_arg3);
    return SUCCESS;
}

/* Custom OnUpdate handler for erasure_coding_shards string */
static ZEND_INI_MH(OnUpdateErasureCodingShards)
{
    int data_shards, parity_shards;
    char d, p;
    if (sscanf(ZSTR_VAL(new_value), "%d%c%d%c", &data_shards, &d, &parity_shards, &p) != 4 || d != 'd' || p != 'p' || data_shards <= 0 || parity_shards <= 0) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Invalid format for erasure coding shards. Expected format like '8d4p' with positive integers.");
        return FAILURE;
    }
    OnUpdateString(entry, new_value, mh_arg1, mh_arg2, mh_arg3);
    return SUCCESS;
}

PHP_INI_BEGIN()
    /* --- General & API --- */
    STD_PHP_INI_ENTRY("quicpro.storage_enable", "0", PHP_INI_SYSTEM, OnUpdateBool, enable, qp_native_object_store_config_t, quicpro_native_object_store_config)
    STD_PHP_INI_ENTRY("quicpro.storage_s3_api_compat_enable", "0", PHP_INI_SYSTEM, OnUpdateBool, s3_api_compat_enable, qp_native_object_store_config_t, quicpro_native_object_store_config)
    STD_PHP_INI_ENTRY("quicpro.storage_versioning_enable", "1", PHP_INI_SYSTEM, OnUpdateBool, versioning_enable, qp_native_object_store_config_t, quicpro_native_object_store_config)
    STD_PHP_INI_ENTRY("quicpro.storage_allow_anonymous_access", "0", PHP_INI_SYSTEM, OnUpdateBool, allow_anonymous_access, qp_native_object_store_config_t, quicpro_native_object_store_config)

    /* --- Data Placement & Redundancy --- */
    ZEND_INI_ENTRY_EX("quicpro.storage_default_redundancy_mode", "erasure_coding", PHP_INI_SYSTEM, OnUpdateRedundancyMode, &quicpro_native_object_store_config.default_redundancy_mode, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.storage_erasure_coding_shards", "8d4p", PHP_INI_SYSTEM, OnUpdateErasureCodingShards, &quicpro_native_object_store_config.erasure_coding_shards, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.storage_default_replication_factor", "3", PHP_INI_SYSTEM, OnUpdateObjectStorePositiveLong, NULL, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.storage_default_chunk_size_mb", "64", PHP_INI_SYSTEM, OnUpdateObjectStorePositiveLong, NULL, NULL, NULL)

    /* --- Cluster Topology & Discovery --- */
    STD_PHP_INI_ENTRY("quicpro.storage_metadata_agent_uri", "127.0.0.1:9701", PHP_INI_SYSTEM, OnUpdateString, metadata_agent_uri, qp_native_object_store_config_t, quicpro_native_object_store_config)
    ZEND_INI_ENTRY_EX("quicpro.storage_node_discovery_mode", "static", PHP_INI_SYSTEM, OnUpdateDiscoveryMode, &quicpro_native_object_store_config.node_discovery_mode, NULL, NULL)
    STD_PHP_INI_ENTRY("quicpro.storage_node_static_list", "127.0.0.1:9711,127.0.0.1:9712", PHP_INI_SYSTEM, OnUpdateString, node_static_list, qp_native_object_store_config_t, quicpro_native_object_store_config)

    /* --- Performance & Caching --- */
    STD_PHP_INI_ENTRY("quicpro.storage_metadata_cache_enable", "1", PHP_INI_SYSTEM, OnUpdateBool, metadata_cache_enable, qp_native_object_store_config_t, quicpro_native_object_store_config)
    ZEND_INI_ENTRY_EX("quicpro.storage_metadata_cache_ttl_sec", "60", PHP_INI_SYSTEM, OnUpdateObjectStorePositiveLong, NULL, NULL, NULL)
    STD_PHP_INI_ENTRY("quicpro.storage_enable_directstorage", "0", PHP_INI_SYSTEM, OnUpdateBool, enable_directstorage, qp_native_object_store_config_t, quicpro_native_object_store_config)
PHP_INI_END()

void qp_config_native_object_store_ini_register(void) { REGISTER_INI_ENTRIES(); }
void qp_config_native_object_store_ini_unregister(void) { UNREGISTER_INI_ENTRIES(); }
