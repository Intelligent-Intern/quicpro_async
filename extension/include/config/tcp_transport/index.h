/*
 * =========================================================================
 * FILENAME:   include/config/tcp_transport/index.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    So, you're saying that there's a pressure-sensitive...
 * fluid breathing system?
 *
 * PURPOSE:
 * This header file declares the public C-API for the TCP Transport
 * configuration module.
 *
 * ARCHITECTURE:
 * It provides the function prototypes for the module's lifecycle, which
 * are called by the master dispatcher to orchestrate loading of settings.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_TCP_TRANSPORT_INDEX_H
#define QUICPRO_CONFIG_TCP_TRANSPORT_INDEX_H

/**
 * @brief Initializes the TCP Transport configuration module.
 */
void qp_config_tcp_transport_init(void);

/**
 * @brief Shuts down the TCP Transport configuration module.
 */
void qp_config_tcp_transport_shutdown(void);

#endif /* QUICPRO_CONFIG_TCP_TRANSPORT_INDEX_H */
