/*
 * =========================================================================
 * FILENAME:   include/config/ssh_over_quic/ini.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    I only lent you my body. You lent me your dream.
 *
 * PURPOSE:
 * This header file declares the internal C-API for handling the `php.ini`
 * entries of the SSH over QUIC configuration module.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_SSH_OVER_QUIC_INI_H
#define QUICPRO_CONFIG_SSH_OVER_QUIC_INI_H

/**
 * @brief Registers this module's php.ini entries with the Zend Engine.
 */
void qp_config_ssh_over_quic_ini_register(void);

/**
 * @brief Unregisters this module's php.ini entries from the Zend Engine.
 */
void qp_config_ssh_over_quic_ini_unregister(void);

#endif /* QUICPRO_CONFIG_SSH_OVER_QUIC_INI_H */
