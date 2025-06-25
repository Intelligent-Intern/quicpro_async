/*
 * =========================================================================
 * FILENAME:   include/config/iibin/ini.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    For a moment, nothing happened. Then, after a second or so,
 * nothing continued to happen.
 *
 * PURPOSE:
 * This header file declares the internal C-API for handling the `php.ini`
 * entries of the IIBIN serialization configuration module.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_IIBIN_INI_H
#define QUICPRO_CONFIG_IIBIN_INI_H

/**
 * @brief Registers this module's php.ini entries with the Zend Engine.
 */
void qp_config_iibin_ini_register(void);

/**
 * @brief Unregisters this module's php.ini entries from the Zend Engine.
 */
void qp_config_iibin_ini_unregister(void);

#endif /* QUICPRO_CONFIG_IIBIN_INI_H */
