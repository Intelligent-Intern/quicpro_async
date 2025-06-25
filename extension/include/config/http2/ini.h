/*
 * =========================================================================
 * FILENAME:   include/config/http2/ini.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    If my calculations are correct, when this baby hits 88 miles
 * per hour... you're gonna see some serious shit.
 *
 * PURPOSE:
 * This header file declares the internal C-API for handling the `php.ini`
 * entries of the HTTP/2 configuration module.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_HTTP2_INI_H
#define QUICPRO_CONFIG_HTTP2_INI_H

/**
 * @brief Registers this module's php.ini entries with the Zend Engine.
 */
void qp_config_http2_ini_register(void);

/**
 * @brief Unregisters this module's php.ini entries from the Zend Engine.
 */
void qp_config_http2_ini_unregister(void);

#endif /* QUICPRO_CONFIG_HTTP2_INI_H */
