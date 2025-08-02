/*
 * =========================================================================
 * FILENAME:   src/config/native_object_store/default.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    All we have to decide is what to do with the time that is
 * given to us.
 *
 * PURPOSE:
 * This file implements the function that loads the hardcoded, default
 * values into the Native Object Store configuration struct.
 * =========================================================================
 */

#include "include/config/native_object_store/default.h"
#include "include/config/native_object_store/base_layer.h"
#include "php.h"

/**
 * @brief Loads the hardcoded, default values into the module's config struct.
 */
void qp_config_native_object_store_defaults_load(void)
{
    /* --- General & API --- */
    quicpro_native_object_store_config.enable = false; /* Disabled by default */
    quicpro_native_object_store_config.s3_api_compat_enable = false;
    quicpro_native_object_store_config.versioning_enable = true;
    quicpro_native_object_store_config.allow_anonymous_access = false;

    /* --- Data Placement & Redundancy --- */
    quicpro_native_object_store_config.default_redundancy_mode = pestrdup("erasure_coding", 1);
    quicpro_native_object_store_config.erasure_coding_shards = pestrdup("8d4p", 1);
    quicpro_native_object_store_config.default_replication_factor = 3;
    quicpro_native_object_store_config.default_chunk_size_mb = 64;

    /* --- Cluster Topology & Discovery --- */
    quicpro_native_object_store_config.metadata_agent_uri = pestrdup("127.0.0.1:9701", 1);
    quicpro_native_object_store_config.node_discovery_mode = pestrdup("static", 1);
    quicpro_native_object_store_config.node_static_list = pestrdup("127.0.0.1:9711,127.0.0.1:9712", 1);

    /* --- Performance & Caching --- */
    quicpro_native_object_store_config.metadata_cache_enable = true;
    quicpro_native_object_store_config.metadata_cache_ttl_sec = 60;
    quicpro_native_object_store_config.enable_directstorage = false;
}
