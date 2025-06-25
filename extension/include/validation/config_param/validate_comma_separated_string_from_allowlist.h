/*
 * =========================================================================
 * FILENAME:   include/validation/config_param/validate_comma_separated_string_from_allowlist.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    You have to understand, most of these people are not ready
 * to be unplugged.
 *
 * PURPOSE:
 * This header declares a validation helper for comma-separated string
 * values where each token must match an allowed set of options.
 * =========================================================================
 */

#ifndef QUICPRO_VALIDATION_COMMA_SEPARATED_STRING_FROM_ALLOWLIST_H
#define QUICPRO_VALIDATION_COMMA_SEPARATED_STRING_FROM_ALLOWLIST_H

#include "php.h"

/**
 * @brief Validates if a zval is a comma-separated string where each token
 * exists in a predefined allow-list.
 * @details This function enforces three strict rules:
 * 1. The zval must be of type IS_STRING.
 * 2. The string is tokenized by commas.
 * 3. Each non-empty token must be present in the `allowed_values` array.
 *
 * @param value The zval to validate.
 * @param allowed_values A NULL-terminated array of strings representing the
 * valid options.
 * @param target A pointer to a char* where the newly allocated, validated
 * string will be stored.
 * @return `SUCCESS` on successful validation, `FAILURE` otherwise.
 */
int qp_validate_comma_separated_string_from_allowlist(zval *value, const char *allowed_values[], char **target);

#endif /* QUICPRO_VALIDATION_COMMA_SEPARATED_STRING_FROM_ALLOWLIST_H */
