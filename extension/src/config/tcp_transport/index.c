/*
 * =========================================================================
 * FILENAME:   src/config/tcp_transport/index.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    "Simplicity is prerequisite for reliability." – Edsger Dijkstra
 *
 * PURPOSE:
 *   Lifecycle glue for the TCP‑Transport configuration module. During
 *   module startup we (1) load defaults and (2) register php.ini handlers.
 *   During shutdown we clean up after ourselves.
 * =========================================================================
 */

#include "include/config/tcp_transport/index.h"
#include "include/config/tcp_transport/default.h"
#include "include/config/tcp_transport/ini.h"
#include "php.h"

void qp_config_tcp_transport_init(void)
{
    /* Step 1: load safe defaults */
    qp_config_tcp_transport_defaults_load();

    /* Step 2: allow php.ini overrides */
    qp_config_tcp_transport_ini_register();
}

void qp_config_tcp_transport_shutdown(void)
{
    qp_config_tcp_transport_ini_unregister();
}
