/*
 * =========================================================================
 * FILENAME:   include/config/tls_and_crypto/default.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Trust no one.
 *
 * PURPOSE:
 * This header declares the function for loading the module's hardcoded
 * default values for the TLS & Crypto settings.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_TLS_CRYPTO_DEFAULT_H
#define QUICPRO_CONFIG_TLS_CRYPTO_DEFAULT_H

/**
 * @brief Loads the hardcoded, default values into the module's config struct.
 */
void qp_config_tls_and_crypto_defaults_load(void);

#endif /* QUICPRO_CONFIG_TLS_CRYPTO_DEFAULT_H */
