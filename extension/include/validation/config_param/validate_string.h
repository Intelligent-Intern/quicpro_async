/*
 * =========================================================================
 * FILENAME:   include/validation/config_param/validate_string.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    "Strings attached? Not with strict validation." — Unknown Core Dev
 *
 * PURPOSE:
 *   Centralised helper to validate *plain* string configuration parameters
 *   originating from PHP userland or ini‑parsing.  The function enforces
 *   that the given zval is of type IS_STRING **without** performing any
 *   implicit casts/juggling.  On success the C‑string is duplicated into
 *   a *persistent* allocation so that the target pointer may be stored in
 *   a long‑living global config struct.
 *
 * ARCHITECTURE:
 *   ‑ If @dest is non‑NULL, the previous buffer is pefreed (if it was
 *     allocated via pemalloc) and replaced by a pestrdup() of the new
 *     value.
 *   ‑ The helper throws an InvalidArgumentException on failure – never
 *     print anything, never leak the supplied string.
 *
 * USAGE EXAMPLE:
 *   if (qp_validate_string(val, &my_cfg_ptr->path) != SUCCESS) {
 *       return FAILURE;
 *   }
 * =========================================================================
 */

#ifndef QUICPRO_VALIDATION_STRING_H
#define QUICPRO_VALIDATION_STRING_H

#include "php.h"

/**
 * Validates that the given zval is a *strict* PHP string (IS_STRING).
 *
 * @param value  The zval to validate.
 * @param dest   If not NULL, receives a *persistent* duplicate of the string.
 *               Existing memory at *dest is freed first using pefree().
 * @return SUCCESS / FAILURE and throws spl_ce_InvalidArgumentException on error.
 */
int qp_validate_string(zval *value, char **dest);

#endif /* QUICPRO_VALIDATION_STRING_H */
