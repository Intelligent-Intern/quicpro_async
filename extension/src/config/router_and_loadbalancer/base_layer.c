/*
 * =========================================================================
 * FILENAME:   src/config/router_and_loadbalancer/base_layer.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Chevron two encoded.
 *
 * PURPOSE:
 * This file provides the single, authoritative definition and memory
 * allocation for the `quicpro_router_loadbalancer_config` global variable.
 * =========================================================================
 */
#include "include/config/router_and_loadbalancer/base_layer.h"

/*
 * This is the actual definition and memory allocation for the module's
 * configuration structure.
 */
qp_router_loadbalancer_config_t quicpro_router_loadbalancer_config;
