/*
 * =========================================================================
 * FILENAME:   src/config/tls_and_crypto/default.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    "Security by default or insecurity by design – choose wisely."
 *
 * PURPOSE:
 *   Loads hard-coded, conservative defaults into the TLS & Crypto config
 *   struct so the server starts in a secure state even without a php.ini.
 * =========================================================================
 */

#include "include/config/tls_and_crypto/default.h"
#include "include/config/tls_and_crypto/base_layer.h"
#include "php.h"

void qp_config_tls_crypto_defaults_load(void)
{
    /* Transport-layer crypto */
    quicpro_tls_crypto_config.tls_verify_peer                    = true;
    quicpro_tls_crypto_config.tls_verify_depth                  = 10;
    quicpro_tls_crypto_config.tls_default_ca_file               = pestrdup("", 1);
    quicpro_tls_crypto_config.tls_default_cert_file             = pestrdup("", 1);
    quicpro_tls_crypto_config.tls_default_key_file              = pestrdup("", 1);
    quicpro_tls_crypto_config.tls_ticket_key_file               = pestrdup("", 1);
    quicpro_tls_crypto_config.tls_ciphers_tls13                 = pestrdup("TLS_AES_128_GCM_SHA256:TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256", 1);
    quicpro_tls_crypto_config.tls_curves                        = pestrdup("P-256:X25519", 1);
    quicpro_tls_crypto_config.tls_session_ticket_lifetime_sec   = 7200;
    quicpro_tls_crypto_config.tls_enable_early_data             = false;
    quicpro_tls_crypto_config.tls_server_0rtt_cache_size        = 100000;
    quicpro_tls_crypto_config.tls_enable_ocsp_stapling          = true;

    /* Expert / potentially insecure options – keep disabled */
    quicpro_tls_crypto_config.tls_enable_ech                    = false;
    quicpro_tls_crypto_config.tls_require_ct_policy             = false;
    quicpro_tls_crypto_config.tls_disable_sni_validation        = false;
    quicpro_tls_crypto_config.transport_disable_encryption      = false;
}
