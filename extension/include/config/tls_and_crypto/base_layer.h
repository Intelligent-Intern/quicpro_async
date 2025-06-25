/*
 * =========================================================================
 * FILENAME:   include/config/tls_and_crypto/base_layer.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    The truth is out there.
 *
 * PURPOSE:
 * This header file defines the core data structure for the
 * `tls_and_crypto` configuration module.
 *
 * ARCHITECTURE:
 * This struct holds all configuration values related to Transport Layer
 * Security (TLS), certificate management, and data encryption.
 * =========================================================================
 */
#ifndef QUICPRO_CONFIG_TLS_CRYPTO_BASE_H
#define QUICPRO_CONFIG_TLS_CRYPTO_BASE_H

#include "php.h"
#include <stdbool.h>

typedef struct _qp_tls_and_crypto_config_t {
    /* --- Transport Layer Encryption (TLS for QUIC & TCP) --- */
    bool tls_verify_peer;
    zend_long tls_verify_depth;
    char *tls_default_ca_file;
    char *tls_default_cert_file;
    char *tls_default_key_file;
    char *tls_ticket_key_file;

    /* -- Cipher & Curve Configuration -- */
    char *tls_ciphers_tls13; /* For QUIC & modern TCP */
    char *tls_ciphers_tls12; /* For legacy TCP / HTTP/2 compatibility */
    char *tls_curves;

    /* -- Session & Handshake Tuning -- */
    zend_long tls_session_ticket_lifetime_sec;
    bool tls_enable_early_data; /* 0-RTT for QUIC */
    zend_long tls_server_0rtt_cache_size;
    bool tls_enable_ocsp_stapling;
    char *tcp_tls_min_version_allowed; /* e.g., "TLSv1.2", "TLSv1.3" */


    /* --- Storage Encryption (Encryption at Rest) --- */
    bool storage_encryption_at_rest_enable;
    char *storage_encryption_algorithm;
    char *storage_encryption_key_path;

    /* --- Application-Level Encryption --- */
    bool mcp_payload_encryption_enable;
    char *mcp_payload_encryption_psk_env_var;

    /* --- Expert / Potentially Insecure Options --- */
    bool tls_enable_ech;
    bool tls_require_ct_policy;
    bool tls_disable_sni_validation;
    bool transport_disable_encryption;

} qp_tls_and_crypto_config_t;

/* The single instance of this module's configuration data */
extern qp_tls_and_crypto_config_t quicpro_tls_and_crypto_config;

#endif /* QUICPRO_CONFIG_TLS_CRYPTO_BASE_H */
