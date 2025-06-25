/*
 * =========================================================================
 * FILENAME:   include/config/router_and_loadbalancer/ini.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Undomesticated equines could not remove me.
 *
 * PURPOSE:
 * This header file declares the internal C-API for handling the `php.ini`
 * entries of the Router & Load Balancer configuration module.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_ROUTER_LOADBALANCER_INI_H
#define QUICPRO_CONFIG_ROUTER_LOADBALANCER_INI_H

/**
 * @brief Registers this module's php.ini entries with the Zend Engine.
 */
void qp_config_router_and_loadbalancer_ini_register(void);

/**
 * @brief Unregisters this module's php.ini entries from the Zend Engine.
 */
void qp_config_router_and_loadbalancer_ini_unregister(void);

#endif /* QUICPRO_CONFIG_ROUTER_LOADBALANCER_INI_H */
