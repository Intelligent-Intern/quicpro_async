/*
 * =========================================================================
 * FILENAME:   src/config/app_http3_websockets_webtransport/default.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    That's all it is. Information. Even a simulated experience
 * or a dream is simultaneous reality and fantasy.
 *
 * PURPOSE:
 * This file implements the function that loads the hardcoded, default
 * values into the application protocols configuration struct.
 * =========================================================================
 */

#include "include/config/app_http3_websockets_webtransport/default.h"
#include "include/config/app_http3_websockets_webtransport/base_layer.h"
#include "php.h"

/**
 * @brief Loads the hardcoded, default values into the module's config struct.
 */
void qp_config_app_http3_websockets_webtransport_defaults_load(void)
{
    /* --- HTTP/3 General Settings --- */
    quicpro_app_protocols_config.http_advertise_h3_alt_svc = true;
    quicpro_app_protocols_config.http_auto_compress = pestrdup("brotli,gzip", 1);
    quicpro_app_protocols_config.h3_max_header_list_size = 65536;
    quicpro_app_protocols_config.h3_qpack_max_table_capacity = 4096;
    quicpro_app_protocols_config.h3_qpack_blocked_streams = 100;
    quicpro_app_protocols_config.h3_server_push_enable = false;
    quicpro_app_protocols_config.http_enable_early_hints = true;

    /* --- WebSocket Protocol Settings --- */
    quicpro_app_protocols_config.websocket_default_max_payload_size = 16777216; /* 16MB */
    quicpro_app_protocols_config.websocket_default_ping_interval_ms = 25000;
    quicpro_app_protocols_config.websocket_handshake_timeout_ms = 5000;

    /* --- WebTransport Protocol Settings --- */
    quicpro_app_protocols_config.webtransport_enable = true;
    quicpro_app_protocols_config.webtransport_max_concurrent_sessions = 10000;
    quicpro_app_protocols_config.webtransport_max_streams_per_session = 256;
}
