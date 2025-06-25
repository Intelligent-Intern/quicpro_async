/*
 * =========================================================================
 * FILENAME:   include/config/state_management/ini.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    We have to go deeper.
 *
 * PURPOSE:
 * This header file declares the internal C-API for handling the `php.ini`
 * entries of the State Management configuration module.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_STATE_MANAGEMENT_INI_H
#define QUICPRO_CONFIG_STATE_MANAGEMENT_INI_H

/**
 * @brief Registers this module's php.ini entries with the Zend Engine.
 */
void qp_config_state_management_ini_register(void);

/**
 * @brief Unregisters this module's php.ini entries from the Zend Engine.
 */
void qp_config_state_management_ini_unregister(void);

#endif /* QUICPRO_CONFIG_STATE_MANAGEMENT_INI_H */
