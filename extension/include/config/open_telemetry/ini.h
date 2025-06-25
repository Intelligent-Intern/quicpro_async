/*
 * =========================================================================
 * FILENAME:   include/config/open_telemetry/ini.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    You see, but you do not observe. The distinction is clear.
 *
 * PURPOSE:
 * This header file declares the internal C-API for handling the `php.ini`
 * entries of the OpenTelemetry configuration module.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_OPEN_TELEMETRY_INI_H
#define QUICPRO_CONFIG_OPEN_TELEMETRY_INI_H

/**
 * @brief Registers this module's php.ini entries with the Zend Engine.
 */
void qp_config_open_telemetry_ini_register(void);

/**
 * @brief Unregisters this module's php.ini entries from the Zend Engine.
 */
void qp_config_open_telemetry_ini_unregister(void);

#endif /* QUICPRO_CONFIG_OPEN_TELEMETRY_INI_H */
