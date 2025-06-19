/*
 * =========================================================================
 * FILENAME:   include/quicpro_globals.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Wubba Lubba Dub Dub!
 *
 * PURPOSE:
 * This header defines the Global State structure for the entire `quicpro_async`
 * extension. It is intended to hold system-wide state that needs to be
 * accessible from multiple modules but is managed by a single, authoritative
 * source. This avoids polluting the C global namespace and centralizes
 * critical state management.
 *
 * USAGE:
 * This file should be included by any module that needs to read the global
 * state. The `quicpro_globals` struct itself is defined and allocated in
 * a corresponding .c file (e.g., `src/quicpro_globals.c` or the main
 * `php_quicpro.c`) to ensure it exists as a single instance across the
 * entire extension.
 * =========================================================================
 */

#ifndef QUICPRO_GLOBALS_H
#define QUICPRO_GLOBALS_H

#include <php.h>
#include <stdbool.h>

/*
 * Defines the globally accessible state for the extension.
 */
typedef struct _quicpro_globals_t {
    /**
     * @brief Master flag indicating if userland code can override php.ini settings.
     *
     * This value is the single source of truth for runtime configuration changes.
     * It is managed exclusively by the `security_and_traffic` module during
     * the MINIT phase and should be treated as read-only thereafter.
     *
     * true:  Overrides via `Quicpro\Config` or Admin API are allowed.
     * false: Overrides are forbidden. The php.ini values are final and enforced.
     */
    bool is_userland_override_allowed;

} quicpro_globals_t;


/**
 * @brief Extern declaration of the single global state instance.
 *
 * This makes the global state variable available to any file that includes
 * this header. The actual memory for this variable must be defined exactly
 * once in a single .c file within the project to satisfy the linker.
 */
extern ZEND_API quicpro_globals_t quicpro_globals;


#endif /* QUICPRO_GLOBALS_H */
