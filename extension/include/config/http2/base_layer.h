/*
 * =========================================================================
 * FILENAME:   include/config/http2/base_layer.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Roads? Where we're going, we don't need roads.
 *
 * PURPOSE:
 * This header file defines the core data structure for the `http2`
 * configuration module.
 *
 * ARCHITECTURE:
 * This struct holds the configuration values for the native HTTP/2
 * protocol engine after they have been loaded.
 * =========================================================================
 */
#ifndef QUICPRO_CONFIG_HTTP2_BASE_H
#define QUICPRO_CONFIG_HTTP2_BASE_H

#include "php.h"
#include <stdbool.h>

typedef struct _qp_http2_config_t {
    /* --- HTTP/2 General Settings --- */
    bool enable;
    zend_long initial_window_size;
    zend_long max_concurrent_streams;
    zend_long max_header_list_size;
    bool enable_push;
    zend_long max_frame_size;

} qp_http2_config_t;

/* The single instance of this module's configuration data */
extern qp_http2_config_t quicpro_http2_config;

#endif /* QUICPRO_CONFIG_HTTP2_BASE_H */
