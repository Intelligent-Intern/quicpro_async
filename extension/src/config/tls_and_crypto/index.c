/*
 * =========================================================================
 * FILENAME:   src/config/tls_and_crypto/index.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    "A secure handshake is worth a thousand hand-wavy excuses."
 *
 * PURPOSE:
 *   Defines the life-cycle entry points for the TLS & Crypto configuration
 *   module: init() at MINIT and shutdown() at MSHUTDOWN.
 * =========================================================================
 */

#include "include/config/tls_and_crypto/index.h"
#include "include/config/tls_and_crypto/default.h"
#include "include/config/tls_and_crypto/ini.h"
#include "php.h"

void qp_config_tls_crypto_init(void)
{
    /* 1. Load hard-coded, secure defaults */
    qp_config_tls_crypto_defaults_load();

    /* 2. Register php.ini directives that may override the defaults */
    qp_config_tls_crypto_ini_register();
}

void qp_config_tls_crypto_shutdown(void)
{
    qp_config_tls_crypto_ini_unregister();
}
