/*
 * =========================================================================
 * FILENAME:   include/config/router_and_loadbalancer/base_layer.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Chevron one encoded.
 *
 * PURPOSE:
 * This header file defines the core data structure for the
 * `router_and_loadbalancer` configuration module.
 *
 * ARCHITECTURE:
 * This struct holds the configuration values for running a server instance
 * in a high-performance, stateless L7 QUIC router mode.
 * =========================================================================
 */
#ifndef QUICPRO_CONFIG_ROUTER_LOADBALANCER_BASE_H
#define QUICPRO_CONFIG_ROUTER_LOADBALANCER_BASE_H

#include "php.h"
#include <stdbool.h>

typedef struct _qp_router_loadbalancer_config_t {
    /* --- Router Mode Activation --- */
    bool router_mode_enable;

    /* --- Backend Routing & Discovery --- */
    char *hashing_algorithm;
    char *connection_id_entropy_salt;
    char *backend_discovery_mode;
    char *backend_static_list;
    char *backend_mcp_endpoint;
    zend_long backend_mcp_poll_interval_sec;

    /* --- Performance & Security --- */
    zend_long max_forwarding_pps;

} qp_router_loadbalancer_config_t;

/* The single instance of this module's configuration data */
extern qp_router_loadbalancer_config_t quicpro_router_loadbalancer_config;

#endif /* QUICPRO_CONFIG_ROUTER_LOADBALANCER_BASE_H */
