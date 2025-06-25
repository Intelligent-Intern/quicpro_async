/*
 * =========================================================================
 * FILENAME:   include/config/tcp_transport/ini.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    We have to go deeper.
 *
 * PURPOSE:
 * This header file declares the internal C-API for handling the `php.ini`
 * entries of the TCP Transport configuration module.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_TCP_TRANSPORT_INI_H
#define QUICPRO_CONFIG_TCP_TRANSPORT_INI_H

/**
 * @brief Registers this module's php.ini entries with the Zend Engine.
 */
void qp_config_tcp_transport_ini_register(void);

/**
 * @brief Unregisters this module's php.ini entries from the Zend Engine.
 */
void qp_config_tcp_transport_ini_unregister(void);

#endif /* QUICPRO_CONFIG_TCP_TRANSPORT_INI_H */
