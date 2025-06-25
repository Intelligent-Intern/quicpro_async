/*
 * =========================================================================
 * FILENAME:   include/config/native_object_store/base_layer.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    One Ring to rule them all...
 *
 * PURPOSE:
 * This header file defines the core data structure for the
 * `native_object_store` configuration module.
 *
 * ARCHITECTURE:
 * This struct holds the configuration values for the built-in, high-
 * performance distributed object store (`quicpro-fs://`).
 * =========================================================================
 */
#ifndef QUICPRO_CONFIG_NATIVE_OBJECT_STORE_BASE_H
#define QUICPRO_CONFIG_NATIVE_OBJECT_STORE_BASE_H

#include "php.h"
#include <stdbool.h>

typedef struct _qp_native_object_store_config_t {
    /* --- General & API --- */
    bool enable;
    bool s3_api_compat_enable;
    bool versioning_enable;
    bool allow_anonymous_access;

    /* --- Data Placement & Redundancy --- */
    char *default_redundancy_mode;
    char *erasure_coding_shards;
    zend_long default_replication_factor;
    zend_long default_chunk_size_mb;

    /* --- Cluster Topology & Discovery --- */
    char *metadata_agent_uri;
    char *node_discovery_mode;
    char *node_static_list;

    /* --- Performance & Caching --- */
    bool metadata_cache_enable;
    zend_long metadata_cache_ttl_sec;
    bool enable_directstorage;

} qp_native_object_store_config_t;

/* The single instance of this module's configuration data */
extern qp_native_object_store_config_t quicpro_native_object_store_config;

#endif /* QUICPRO_CONFIG_NATIVE_OBJECT_STORE_BASE_H */
