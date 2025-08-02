/*
 * =========================================================================
 * FILENAME:   src/config/tcp_transport/base_layer.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    "The ship is safest when it is in port, but that’s not what ships were built for." – Grace Hopper
 *
 * PURPOSE:
 *   Provides the single, authoritative definition (and static storage) of
 *   the global configuration structure for the TCP-Transport module.
 * =========================================================================
 */

#include "include/config/tcp_transport/base_layer.h"

/*
 * Allocate the one-and-only instance that holds all effective settings
 * for the TCP transport layer.
 */
qp_tcp_transport_config_t quicpro_tcp_transport_config;