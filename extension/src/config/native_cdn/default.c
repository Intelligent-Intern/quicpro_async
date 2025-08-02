/*
 * =========================================================================
 * FILENAME:   src/config/native_cdn/default.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    I kept dreaming of a world I thought I'd never see.
 * And then, one day... I got in.
 *
 * PURPOSE:
 * This file implements the function that loads the hardcoded, default
 * values into the Native CDN configuration struct.
 * =========================================================================
 */

#include "include/config/native_cdn/default.h"
#include "include/config/native_cdn/base_layer.h"
#include "php.h"

/**
 * @brief Loads the hardcoded, default values into the module's config struct.
 */
void qp_config_native_cdn_defaults_load(void)
{
    /* --- General --- */
    quicpro_native_cdn_config.enable = false; /* Disabled by default */

    /* --- Caching Policy & Behavior --- */
    quicpro_native_cdn_config.cache_mode = pestrdup("disk", 1);
    quicpro_native_cdn_config.cache_memory_limit_mb = 512;
    quicpro_native_cdn_config.cache_disk_path = pestrdup("/var/cache/quicpro_cdn", 1);
    quicpro_native_cdn_config.cache_default_ttl_sec = 86400; /* 24 hours */
    quicpro_native_cdn_config.cache_max_object_size_mb = 1024; /* 1GB */
    quicpro_native_cdn_config.cache_respect_origin_headers = true;
    quicpro_native_cdn_config.cache_vary_on_headers = pestrdup("Accept-Encoding", 1);

    /* --- Origin (Backend) Configuration --- */
    quicpro_native_cdn_config.origin_mcp_endpoint = pestrdup("", 1);
    quicpro_native_cdn_config.origin_http_endpoint = pestrdup("", 1);
    quicpro_native_cdn_config.origin_request_timeout_ms = 15000;

    /* --- Client-Facing Behavior --- */
    quicpro_native_cdn_config.serve_stale_on_error = true;
    quicpro_native_cdn_config.response_headers_to_add = pestrdup("X-Cache-Status: HIT", 1);
    quicpro_native_cdn_config.allowed_http_methods = pestrdup("GET,HEAD", 1);
}
