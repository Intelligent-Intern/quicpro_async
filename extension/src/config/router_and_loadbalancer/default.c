/*
 * =========================================================================
 * FILENAME:   src/config/router_and_loadbalancer/default.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Chevron four encoded.
 *
 * PURPOSE:
 * This file implements the function that loads the hardcoded, default
 * values into the Router & Load Balancer configuration struct.
 * =========================================================================
 */

#include "include/config/router_and_loadbalancer/default.h"
#include "include/config/router_and_loadbalancer/base_layer.h"
#include "php.h"

/**
 * @brief Loads the hardcoded, default values into the module's config struct.
 */
void qp_config_router_and_loadbalancer_defaults_load(void)
{
    /* --- Router Mode Activation --- */
    quicpro_router_loadbalancer_config.router_mode_enable = false;

    /* --- Backend Routing & Discovery --- */
    quicpro_router_loadbalancer_config.hashing_algorithm = pestrdup("consistent_hash", 1);
    quicpro_router_loadbalancer_config.connection_id_entropy_salt = pestrdup("change-this-to-a-long-random-string-in-production", 1);
    quicpro_router_loadbalancer_config.backend_discovery_mode = pestrdup("static", 1);
    quicpro_router_loadbalancer_config.backend_static_list = pestrdup("127.0.0.1:8443", 1);
    quicpro_router_loadbalancer_config.backend_mcp_endpoint = pestrdup("127.0.0.1:9998", 1);
    quicpro_router_loadbalancer_config.backend_mcp_poll_interval_sec = 10;

    /* --- Performance & Security --- */
    quicpro_router_loadbalancer_config.max_forwarding_pps = 1000000;
}
