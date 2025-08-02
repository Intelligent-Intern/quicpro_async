/*
 * =========================================================================
 * FILENAME:   src/config/quic_transport/index.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Dinosaurs eat man. Woman inherits the earth.
 *
 * PURPOSE:
 * This file implements the lifecycle functions for the QUIC Transport
 * configuration module.
 * =========================================================================
 */

#include "include/config/quic_transport/index.h"
#include "include/config/quic_transport/default.h"
#include "include/config/quic_transport/ini.h"
#include "php.h"

/**
 * @brief Initializes the QUIC Transport configuration module.
 */
void qp_config_quic_transport_init(void)
{
    /* Step 1: Load hardcoded, safe defaults. */
    qp_config_quic_transport_defaults_load();

    /* Step 2: Register INI handlers to override defaults. */
    qp_config_quic_transport_ini_register();
}

/**
 * @brief Shuts down the QUIC Transport configuration module.
 */
void qp_config_quic_transport_shutdown(void)
{
    qp_config_quic_transport_ini_unregister();
}
