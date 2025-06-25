/*
 * =========================================================================
 * FILENAME:   include/config/ssh_over_quic/index.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    There is no gene for the human spirit.
 *
 * PURPOSE:
 * This header file declares the public C-API for the SSH over QUIC
 * configuration module.
 *
 * ARCHITECTURE:
 * It provides the function prototypes for the module's lifecycle, which
 * are called by the master dispatcher to orchestrate loading of settings.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_SSH_OVER_QUIC_INDEX_H
#define QUICPRO_CONFIG_SSH_OVER_QUIC_INDEX_H

/**
 * @brief Initializes the SSH over QUIC configuration module.
 */
void qp_config_ssh_over_quic_init(void);

/**
 * @brief Shuts down the SSH over QUIC configuration module.
 */
void qp_config_ssh_over_quic_shutdown(void);

#endif /* QUICPRO_CONFIG_SSH_OVER_QUIC_INDEX_H */
