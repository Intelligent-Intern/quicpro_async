/*
 * =========================================================================
 * FILENAME:   src/config/ssh_over_quic/index.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    "Choice. The problem is choice."
 *
 * PURPOSE:
 *   Provides the lifecycle entry‑points for the SSH‑over‑QUIC configuration
 *   module (INIT and SHUTDOWN hooks).
 * =========================================================================
 */

#include "include/config/ssh_over_quic/index.h"
#include "include/config/ssh_over_quic/default.h"
#include "include/config/ssh_over_quic/ini.h"
#include "php.h"

void qp_config_ssh_over_quic_init(void)
{
    /* 1. Hardcoded, safe defaults */
    qp_config_ssh_over_quic_defaults_load();

    /* 2. php.ini overrides */
    qp_config_ssh_over_quic_ini_register();
}

void qp_config_ssh_over_quic_shutdown(void)
{
    qp_config_ssh_over_quic_ini_unregister();
}
