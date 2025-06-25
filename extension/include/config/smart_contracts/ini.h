/*
 * =========================================================================
 * FILENAME:   include/config/smart_contracts/ini.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Sometimes, in order to see the light, you have to risk the dark.
 *
 * PURPOSE:
 * This header file declares the internal C-API for handling the `php.ini`
 * entries of the Smart Contracts configuration module.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_SMART_CONTRACTS_INI_H
#define QUICPRO_CONFIG_SMART_CONTRACTS_INI_H

/**
 * @brief Registers this module's php.ini entries with the Zend Engine.
 */
void qp_config_smart_contracts_ini_register(void);

/**
 * @brief Unregisters this module's php.ini entries from the Zend Engine.
 */
void qp_config_smart_contracts_ini_unregister(void);

#endif /* QUICPRO_CONFIG_SMART_CONTRACTS_INI_H */
