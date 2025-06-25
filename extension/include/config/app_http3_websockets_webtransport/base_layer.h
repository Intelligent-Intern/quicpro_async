/*
 * =========================================================================
 * FILENAME:   include/config/app_http3_websockets_webtransport/base_layer.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Man is an individual only because of his intangible memory...
 * and memory cannot be defined, but it defines mankind.
 *
 * PURPOSE:
 * This header file defines the core data structure for the
 * `app_http3_websockets_webtransport` configuration module.
 *
 * ARCHITECTURE:
 * This struct holds the configuration values for the high-level application
 * protocols (HTTP/3, WebSockets, WebTransport) after they have been loaded.
 * =========================================================================
 */
#ifndef QUICPRO_CONFIG_APP_PROTOCOLS_BASE_H
#define QUICPRO_CONFIG_APP_PROTOCOLS_BASE_H

#include "php.h"
#include <stdbool.h>

typedef struct _qp_app_protocols_config_t {
    /* --- HTTP/3 General Settings --- */
    bool http_advertise_h3_alt_svc;
    char *http_auto_compress;
    zend_long h3_max_header_list_size;
    zend_long h3_qpack_max_table_capacity;
    zend_long h3_qpack_blocked_streams;
    bool h3_server_push_enable;
    bool http_enable_early_hints;

    /* --- WebSocket Protocol Settings --- */
    zend_long websocket_default_max_payload_size;
    zend_long websocket_default_ping_interval_ms;
    zend_long websocket_handshake_timeout_ms;

    /* --- WebTransport Protocol Settings --- */
    bool webtransport_enable;
    zend_long webtransport_max_concurrent_sessions;
    zend_long webtransport_max_streams_per_session;

} qp_app_protocols_config_t;

/* The single instance of this module's configuration data */
extern qp_app_protocols_config_t quicpro_app_protocols_config;

#endif /* QUICPRO_CONFIG_APP_PROTOCOLS_BASE_H */
