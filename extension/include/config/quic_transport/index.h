/*
 * =========================================================================
 * FILENAME:   include/config/quic_transport/index.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Hold on to your butts.
 *
 * PURPOSE:
 * This header file declares the public C-API for the QUIC Transport
 * configuration module.
 *
 * ARCHITECTURE:
 * It provides the function prototypes for the module's lifecycle, which
 * are called by the master dispatcher to orchestrate loading of settings.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_QUIC_TRANSPORT_INDEX_H
#define QUICPRO_CONFIG_QUIC_TRANSPORT_INDEX_H

/**
 * @brief Initializes the QUIC Transport configuration module.
 */
void qp_config_quic_transport_init(void);

/**
 * @brief Shuts down the QUIC Transport configuration module.
 */
void qp_config_quic_transport_shutdown(void);

#endif /* QUICPRO_CONFIG_QUIC_TRANSPORT_INDEX_H */
