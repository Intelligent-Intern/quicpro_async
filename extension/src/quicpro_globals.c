/*
 * =========================================================================
 * FILENAME:   src/quicpro_globals.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    And that's the wayyy the news goes!
 *
 * PURPOSE:
 * This file provides the single, authoritative definition and memory
 * allocation for the global state variable, `quicpro_globals`, which is
 * declared as `extern` in `include/quicpro_globals.h`.
 *
 * ARCHITECTURE:
 * In C, an `extern` variable declared in a header file must be defined
 * exactly once in a single source file across the entire project. This
 * file fulfills that requirement, ensuring that all other modules share
 * the same instance of the global state, thus preventing linker errors
 * related to multiple definitions.
 *
 * USAGE:
 * This file should not typically be modified. It serves only to allocate
 * memory for the global state. All logic pertaining to the manipulation
 * of this state should reside in the appropriate authoritative module
 * (e.g., the security module setting the override flag).
 * =========================================================================
 */

#include "include/quicpro_globals.h"

/*
 * This is the actual definition and memory allocation for the global
 * state structure. The ZEND_API macro ensures it has the correct linkage
 * to be visible across the Zend Engine and other parts of the extension.
 */
ZEND_API quicpro_globals_t quicpro_globals;
