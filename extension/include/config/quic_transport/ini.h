/*
 * =========================================================================
 * FILENAME:   include/config/quic_transport/ini.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Must go faster.
 *
 * PURPOSE:
 * This header file declares the internal C-API for handling the `php.ini`
 * entries of the QUIC Transport configuration module.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_QUIC_TRANSPORT_INI_H
#define QUICPRO_CONFIG_QUIC_TRANSPORT_INI_H

/**
 * @brief Registers this module's php.ini entries with the Zend Engine.
 */
void qp_config_quic_transport_ini_register(void);

/**
 * @brief Unregisters this module's php.ini entries from the Zend Engine.
 */
void qp_config_quic_transport_ini_unregister(void);

#endif /* QUICPRO_CONFIG_QUIC_TRANSPORT_INI_H */
