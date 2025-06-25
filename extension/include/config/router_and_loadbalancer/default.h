/*
 * =========================================================================
 * FILENAME:   include/config/router_and_loadbalancer/default.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Indeed.
 *
 * PURPOSE:
 * This header declares the function for loading the module's hardcoded
 * default values for the Router & Load Balancer mode.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_ROUTER_LOADBALANCER_DEFAULT_H
#define QUICPRO_CONFIG_ROUTER_LOADBALANCER_DEFAULT_H

/**
 * @brief Loads the hardcoded, default values into the module's config struct.
 */
void qp_config_router_and_loadbalancer_defaults_load(void);

#endif /* QUICPRO_CONFIG_ROUTER_LOADBALANCER_DEFAULT_H */
