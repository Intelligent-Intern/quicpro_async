/*
 * =========================================================================
 * FILENAME:   include/quicpro_init.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:  Qapla'!
 *
 * PURPOSE:
 *
 * This header file defines the public C-API for the master dispatcher
 * responsible for managing the `quicpro_async` extension's lifecycle.
 * It declares the functions that will be implemented in `src/quicpro_init.c`.
 *
 * ARCHITECTURE:
 * This file provides the function prototypes that act as the contract
 * between the main extension file (`php_quicpro.c`) and the master
 * dispatcher implementation. These functions are called directly from the
 * PHP lifecycle macros (MINIT, MSHUTDOWN, etc.).
 *
 * AUDIT & REVIEW NOTES:
 * This file represents the public API for the extension's configuration
 * lifecycle management. It is a high-impact, critical path file.
 * Any changes to function signatures here have wide-ranging consequences:
 *
 * 1. Deep Understanding Required: As these settings are consumed by the
 * core functional modules, any change requires a deep understanding
 * of the underlying implementation that uses the configuration.
 *
 * 2. Formal Change Control: A formal issue must be opened and accepted
 * by the maintainers before any modifications are made.
 *
 * 3. Cascade of Changes: A change here necessitates corresponding changes
 * in `src/quicpro_init.c`, the main `php_quicpro.c` file, AND all
 * functional modules where the associated configuration is consumed.
 *
 * =========================================================================
 */

#ifndef QUICPRO_INIT_H
#define QUICPRO_INIT_H

#include <php.h>

/**
 * @brief Declares the master dispatcher for registering all module configurations.
 * @details This function, implemented in `quicpro_init.c`, will be called from
 * `PHP_MINIT_FUNCTION`. It orchestrates the registration of all `php.ini`
 * directives by calling the `_init()` function from each config module.
 *
 * @param type The type of initialization (e.g., MODULE_PERSISTENT).
 * @param module_number The unique number assigned to this extension by Zend.
 * @return `SUCCESS` on successful registration, `FAILURE` otherwise.
 */
int quicpro_init_modules(int type, int module_number);

/**
 * @brief Declares the master dispatcher for unregistering all module configurations.
 * @details This function, implemented in `quicpro_init.c`, will be called from
 * `PHP_MSHUTDOWN_FUNCTION`. It orchestrates the clean shutdown of all
 * configuration modules by unregistering their INI entries.
 *
 * CRITICAL: The implementation must unregister handlers in the exact
 * reverse order of their registration to prevent errors.
 *
 * @param type The type of shutdown (e.g., MODULE_PERSISTENT).
 * @param module_number The module number.
 * @return `SUCCESS` on successful shutdown, `FAILURE` otherwise.
 */
int quicpro_shutdown_modules(int type, int module_number);

/**
 * @brief Declares the master request initialization function.
 * @details This function will be called from `PHP_RINIT_FUNCTION` at the
 * beginning of each PHP request. It provides a hook for initializing
 * any per-request state or resources.
 */
int quicpro_request_init(int type, int module_number);

/**
 * @brief Declares the master request shutdown function.
 * @details This function will be called from `PHP_RSHUTDOWN_FUNCTION` at the end
 * of each PHP request. It provides a hook for cleaning up any
 * per-request state or resources to prevent memory leaks.
 */
int quicpro_request_shutdown(int type, int module_number);

#endif // QUICPRO_INIT_H
