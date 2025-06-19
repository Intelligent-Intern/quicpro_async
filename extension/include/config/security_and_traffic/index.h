/*
 * =========================================================================
 * FILENAME:   include/config/security_and_traffic/index.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    https://www.youtube.com/watch?v=dQw4w9WgXcQ
 *
 * PURPOSE:
 * This header file declares the public C-API for the `security_and_traffic`
 * configuration module.
 *
 * ARCHITECTURE:
 * It provides the function prototypes for the module's lifecycle, which
 * are called by the master dispatcher (`quicpro_init.c`) during the
 * `MINIT` and `MSHUTDOWN` phases. These functions are responsible for
 * registering and unregistering the module's specific `php.ini` directives.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_SECURITY_INDEX_H
#define QUICPRO_CONFIG_SECURITY_INDEX_H

/**
 * @brief Initializes the Security & Traffic configuration module.
 * @details This function orchestrates the loading sequence for security-
 * related settings. It first loads hardcoded, safe defaults, and
 * then registers the INI handlers which allow the Zend Engine to
 * override them with values from `php.ini`. Most importantly, this
 * process establishes the global security policy for userland overrides.
 * It is called from `quicpro_init_modules()` at startup.
 */
void qp_config_security_and_traffic_init(void);

/**
 * @brief Shuts down the Security & Traffic configuration module.
 * @details This function unregisters the module's `php.ini` settings
 * to ensure a clean shutdown. It is called from
 * `quicpro_shutdown_modules()` during the MSHUTDOWN phase.
 */
void qp_config_security_and_traffic_shutdown(void);

#endif /* QUICPRO_CONFIG_SECURITY_INDEX_H */
