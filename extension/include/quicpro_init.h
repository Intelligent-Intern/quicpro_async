/*
 * =========================================================================
 * FILENAME:   include/quicpro_init.h
 * MODULE:     quicpro_async: Master Module Initializer
 * =========================================================================
 *
 * PURPOSE:
 * This header file defines the public C-API for the master initialization
 * and shutdown sequence of the entire `quicpro_async` extension.
 *
 * Its functions are called directly from the main `php_quicpro.c` file
 * during the `MINIT`/`MSHUTDOWN` and `RINIT`/`RSHUTDOWN` phases of the
 * PHP lifecycle.
 *
 * ARCHITECTURE:
 * This module acts as a high-level dispatcher. The `quicpro_init_modules()`
 * function is responsible for calling the individual initialization
 * function from every logical sub-module in the framework (e.g., from each
 * of the ~21 configuration modules, server modules, etc.). This enforces a
 * clean, modular startup and shutdown sequence for the entire extension.
 *
 * =========================================================================
 */

#ifndef QUICPRO_INIT_H
#define QUICPRO_INIT_H

#include <php.h>

/**
 * @brief Master initialization function for all framework modules.
 *
 * This function is called from `PHP_MINIT_FUNCTION(quicpro_async)`. It
 * orchestrates the startup of all sub-modules by calling their respective
 * `_init()` functions. This includes registering all classes, resources,
 * and php.ini directives.
 *
 * @param type The type of initialization (MODULE_startup, etc.).
 * @param module_number The unique number assigned to this extension by Zend.
 * @return SUCCESS or FAILURE.
 */
int quicpro_init_modules(int type, int module_number);

/**
 * @brief Master shutdown function for all framework modules.
 *
 * This function is called from `PHP_MSHUTDOWN_FUNCTION(quicpro_async)`.
 * It orchestrates the clean shutdown of all sub-modules.
 *
 * @param type The type of shutdown (MODULE_shutdown, etc.).
 * @param module_number The module number.
 * @return SUCCESS or FAILURE.
 */
int quicpro_shutdown_modules(int type, int module_number);


/**
 * @brief Master request initialization function.
 *
 * Called from `PHP_RINIT_FUNCTION(quicpro_async)` at the beginning of each
 * PHP request.
 */
int quicpro_request_init(int type, int module_number);


/**
 * @brief Master request shutdown function.
 *
 * Called from `PHP_RSHUTDOWN_FUNCTION(quicpro_async)` at the end of each
 * PHP request.
 */
int quicpro_request_shutdown(int type, int module_number);

#endif // QUICPRO_INIT_H