/*
 * =========================================================================
 * FILENAME:   include/config/tls_and_crypto/index.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    I want to believe.
 *
 * PURPOSE:
 * This header file declares the public C-API for the TLS & Crypto
 * configuration module.
 *
 * ARCHITECTURE:
 * It provides the function prototypes for the module's lifecycle, which
 * are called by the master dispatcher to orchestrate loading of settings.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_TLS_CRYPTO_INDEX_H
#define QUICPRO_CONFIG_TLS_CRYPTO_INDEX_H

/**
 * @brief Initializes the TLS & Crypto configuration module.
 */
void qp_config_tls_and_crypto_init(void);

/**
 * @brief Shuts down the TLS & Crypto configuration module.
 */
void qp_config_tls_and_crypto_shutdown(void);

#endif /* QUICPRO_CONFIG_TLS_CRYPTO_INDEX_H */
