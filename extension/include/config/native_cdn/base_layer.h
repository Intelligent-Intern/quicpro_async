/*
 * =========================================================================
 * FILENAME:   include/config/native_cdn/base_layer.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Greetings, program.
 *
 * PURPOSE:
 * This header file defines the core data structure for the
 * `native_cdn` (Content Delivery Network) configuration module.
 *
 * ARCHITECTURE:
 * This struct holds the configuration values for running a server instance
 * as a high-performance, caching reverse proxy or CDN Edge Node.
 * =========================================================================
 */
#ifndef QUICPRO_CONFIG_NATIVE_CDN_BASE_H
#define QUICPRO_CONFIG_NATIVE_CDN_BASE_H

#include "php.h"
#include <stdbool.h>

typedef struct _qp_native_cdn_config_t {
    /* --- General --- */
    bool enable;

    /* --- Caching Policy & Behavior --- */
    char *cache_mode;
    zend_long cache_memory_limit_mb;
    char *cache_disk_path;
    zend_long cache_default_ttl_sec;
    zend_long cache_max_object_size_mb;
    bool cache_respect_origin_headers;
    char *cache_vary_on_headers;

    /* --- Origin (Backend) Configuration --- */
    char *origin_mcp_endpoint;
    char *origin_http_endpoint;
    zend_long origin_request_timeout_ms;

    /* --- Client-Facing Behavior --- */
    bool serve_stale_on_error;
    char *response_headers_to_add;
    char *allowed_http_methods;

} qp_native_cdn_config_t;

/* The single instance of this module's configuration data */
extern qp_native_cdn_config_t quicpro_native_cdn_config;

#endif /* QUICPRO_CONFIG_NATIVE_CDN_BASE_H */
