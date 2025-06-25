/*
 * =========================================================================
 * FILENAME:   include/config/tcp_transport/base_layer.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    We all see what we want to see. Coffey looks and he sees
 * a hydro-thermal vent.
 *
 * PURPOSE:
 * This header file defines the core data structure for the `tcp_transport`
 * configuration module.
 *
 * ARCHITECTURE:
 * This struct holds the configuration values for the generic, C-native
 * TCP server and client functionality.
 * =========================================================================
 */
#ifndef QUICPRO_CONFIG_TCP_TRANSPORT_BASE_H
#define QUICPRO_CONFIG_TCP_TRANSPORT_BASE_H

#include "php.h"
#include <stdbool.h>

typedef struct _qp_tcp_transport_config_t {
    /* --- Connection Management --- */
    bool enable;
    zend_long max_connections;
    zend_long connect_timeout_ms;
    zend_long listen_backlog;
    bool reuse_port_enable;

    /* --- Latency & Throughput (Nagle's Algorithm) --- */
    bool nodelay_enable;
    bool cork_enable;

    /* --- Keep-Alive Settings --- */
    bool keepalive_enable;
    zend_long keepalive_time_sec;
    zend_long keepalive_interval_sec;
    zend_long keepalive_probes;

    /* --- TLS over TCP Settings --- */
    char *tls_min_version_allowed;
    char *tls_ciphers_tls12;

} qp_tcp_transport_config_t;

/* The single instance of this module's configuration data */
extern qp_tcp_transport_config_t quicpro_tcp_transport_config;

#endif /* QUICPRO_CONFIG_TCP_TRANSPORT_BASE_H */
