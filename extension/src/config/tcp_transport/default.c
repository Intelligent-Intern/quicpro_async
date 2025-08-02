/*
 * =========================================================================
 * FILENAME:   src/config/tcp_transport/default.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    "Good defaults make great software." – Unknown
 *
 * PURPOSE:
 *   Loads the hard-coded, *conservative* default values into the TCP-Transport
 *   configuration struct. These defaults are chosen to be safe on commodity
 *   hardware while still delivering respectable performance.
 * =========================================================================
 */

#include "include/config/tcp_transport/default.h"
#include "include/config/tcp_transport/base_layer.h"
#include "php.h"

void qp_config_tcp_transport_defaults_load(void)
{
    /* --- Connection management --- */
    quicpro_tcp_transport_config.tcp_enable                  = true;
    quicpro_tcp_transport_config.tcp_max_connections         = 10240;
    quicpro_tcp_transport_config.tcp_connect_timeout_ms      = 5000;
    quicpro_tcp_transport_config.tcp_listen_backlog          = 511;

    /* --- Socket options --- */
    quicpro_tcp_transport_config.tcp_reuse_port_enable       = true;
    quicpro_tcp_transport_config.tcp_nodelay_enable          = false;
    quicpro_tcp_transport_config.tcp_cork_enable             = false;

    /* --- Keep-Alive --- */
    quicpro_tcp_transport_config.tcp_keepalive_enable        = true;
    quicpro_tcp_transport_config.tcp_keepalive_time_sec      = 7200;
    quicpro_tcp_transport_config.tcp_keepalive_interval_sec  = 75;
    quicpro_tcp_transport_config.tcp_keepalive_probes        = 9;

    /* --- TLS over TCP --- */
    quicpro_tcp_transport_config.tcp_tls_min_version_allowed = pestrdup("TLSv1.2", 1);
    quicpro_tcp_transport_config.tcp_tls_ciphers_tls12       = pestrdup("ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384", 1);
}