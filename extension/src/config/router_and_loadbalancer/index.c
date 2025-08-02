/*
 * =========================================================================
 * FILENAME:   src/config/router_and_loadbalancer/index.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Chevron three encoded.
 *
 * PURPOSE:
 * This file implements the lifecycle functions for the Router & Load
 * Balancer configuration module.
 * =========================================================================
 */

#include "include/config/router_and_loadbalancer/index.h"
#include "include/config/router_and_loadbalancer/default.h"
#include "include/config/router_and_loadbalancer/ini.h"
#include "php.h"

/**
 * @brief Initializes the Router & Load Balancer configuration module.
 */
void qp_config_router_and_loadbalancer_init(void)
{
    /* Step 1: Load hardcoded, safe defaults. */
    qp_config_router_and_loadbalancer_defaults_load();

    /* Step 2: Register INI handlers to override defaults. */
    qp_config_router_and_loadbalancer_ini_register();
}

/**
 * @brief Shuts down the Router & Load Balancer configuration module.
 */
void qp_config_router_and_loadbalancer_shutdown(void)
{
    qp_config_router_and_loadbalancer_ini_unregister();
}
