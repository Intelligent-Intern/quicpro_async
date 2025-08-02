/*
 * =========================================================================
 * FILENAME:   src/config/tls_and_crypto/base_layer.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    "In crypto we trust – all others must bring entropy."
 *
 * PURPOSE:
 *   Provides the single, authoritative definition & memory allocation for
 *   the global TLS & Crypto configuration structure.
 * =========================================================================
 */

#include "include/config/tls_and_crypto/base_layer.h"

/*
 * The one-and-only instance holding the module's live configuration.
 */
qp_tls_crypto_config_t quicpro_tls_crypto_config;
