/*
 * =========================================================================
 * FILENAME:   include/validation/config_param/validate_colon_separated_string_from_allowlist.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    "All input is evil until proven otherwise." — Every senior pentester
 *
 * PURPOSE:
 *   Declares qp_validate_colon_separated_string_from_allowlist(), a reusable
 *   helper that validates colon‑separated strings (e.g. TLS cipher lists)
 *   against a predefined allow‑list of tokens.  It ensures *every* token in
 *   the list is a member of the allow‑list and, on success, stores a
 *   persistent copy (PESTRDUP) into the provided destination pointer.
 * =========================================================================
 */
#ifndef QUICPRO_VALIDATION_COLON_SEPARATED_STRING_FROM_ALLOWLIST_H
#define QUICPRO_VALIDATION_COLON_SEPARATED_STRING_FROM_ALLOWLIST_H

#include "php.h"

/**
 * @brief Validates that a zval contains a colon‑separated list of tokens, all
 *        of which are present in the supplied allow‑list.
 *
 * @param value   The zval containing the string to validate.
 * @param allowed NULL‑terminated array of *lower‑case* tokens that are
 *                considered valid (e.g. {"tls_AES_128_GCM_SHA256", …, NULL}).
 * @param dest    If non‑NULL and validation succeeds, receives a pestrdup()'d
 *                copy of the *original* string.  If dest already points to a
 *                previous allocation it is *not* freed – the caller is in
 *                charge of lifetime management to avoid double‑frees when
 *                settings are re‑applied.
 *
 * @return SUCCESS on success, FAILURE otherwise (and throws
 *         InvalidArgumentException on error).
 */
int qp_validate_colon_separated_string_from_allowlist(zval *value, const char *allowed[], char **dest);

#endif /* QUICPRO_VALIDATION_COLON_SEPARATED_STRING_FROM_ALLOWLIST_H */