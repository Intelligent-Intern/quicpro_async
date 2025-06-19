/*
 * =========================================================================
 * FILENAME:   include/config/security_and_traffic/ini.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Never gonna make you cry...
 *
 * PURPOSE:
 * This header file declares the internal C-API for handling the `php.ini`
 * entries of the `security_and_traffic` configuration module.
 *
 * ARCHITECTURE:
 * These functions are considered internal to the `security_and_traffic`
 * module. They are called by the module's `index.c` orchestrator and
 * provide the direct interface to the Zend Engine's INI system by
 * wrapping the `REGISTER_INI_ENTRIES` and `UNREGISTER_INI_ENTRIES` macros.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_SECURITY_INI_H
#define QUICPRO_CONFIG_SECURITY_INI_H

/**
 * @brief Registers this module's php.ini entries with the Zend Engine.
 * @details This function is called by the module's index during the MINIT
 * phase. It makes all security-related INI directives known to PHP and
 * triggers the custom OnUpdate handlers that perform validation and set
 * the global security policy.
 */
void qp_config_security_ini_register(void);

/**
 * @brief Unregisters this module's php.ini entries from the Zend Engine.
 * @details This function is called by the module's index during the
 * MSHUTDOWN phase to ensure a clean release of resources.
 */
void qp_config_security_ini_unregister(void);

#endif /* QUICPRO_CONFIG_SECURITY_INI_H */
