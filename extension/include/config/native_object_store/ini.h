/*
 * =========================================================================
 * FILENAME:   include/config/native_object_store/ini.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    ...and in the darkness bind them.
 *
 * PURPOSE:
 * This header file declares the internal C-API for handling the `php.ini`
 * entries of the Native Object Store configuration module.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_NATIVE_OBJECT_STORE_INI_H
#define QUICPRO_CONFIG_NATIVE_OBJECT_STORE_INI_H

/**
 * @brief Registers this module's php.ini entries with the Zend Engine.
 */
void qp_config_native_object_store_ini_register(void);

/**
 * @brief Unregisters this module's php.ini entries from the Zend Engine.
 */
void qp_config_native_object_store_ini_unregister(void);

#endif /* QUICPRO_CONFIG_NATIVE_OBJECT_STORE_INI_H */
