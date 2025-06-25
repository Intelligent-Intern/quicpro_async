/*
 * =========================================================================
 * FILENAME:   include/config/native_cdn/ini.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    This is a digital frontier.
 *
 * PURPOSE:
 * This header file declares the internal C-API for handling the `php.ini`
 * entries of the Native CDN configuration module.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_NATIVE_CDN_INI_H
#define QUICPRO_CONFIG_NATIVE_CDN_INI_H

/**
 * @brief Registers this module's php.ini entries with the Zend Engine.
 */
void qp_config_native_cdn_ini_register(void);

/**
 * @brief Unregisters this module's php.ini entries from the Zend Engine.
 */
void qp_config_native_cdn_ini_unregister(void);

#endif /* QUICPRO_CONFIG_NATIVE_CDN_INI_H */
