/*
 * =========================================================================
 * FILENAME:   include/config/dynamic_admin_api/ini.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    It's a Sicilian message. It means Luca Brasi sleeps
 * with the fishes.
 *
 * PURPOSE:
 * This header file declares the internal C-API for handling the `php.ini`
 * entries of the dynamic admin API configuration module.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_DYNAMIC_ADMIN_API_INI_H
#define QUICPRO_CONFIG_DYNAMIC_ADMIN_API_INI_H

/**
 * @brief Registers this module's php.ini entries with the Zend Engine.
 */
void qp_config_dynamic_admin_api_ini_register(void);

/**
 * @brief Unregisters this module's php.ini entries from the Zend Engine.
 */
void qp_config_dynamic_admin_api_ini_unregister(void);

#endif /* QUICPRO_CONFIG_DYNAMIC_ADMIN_API_INI_H */
