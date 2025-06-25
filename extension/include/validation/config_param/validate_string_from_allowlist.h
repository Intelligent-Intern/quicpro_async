/*
 * =========================================================================
 * FILENAME:   include/validation/config_param/validate_string_from_allowlist.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    You take the red pill...
 *
 * PURPOSE:
 * This header declares a centralized, reusable validation helper function
 * for string values that must match one of an allowed set of options.
 * =========================================================================
 */

#ifndef QUICPRO_VALIDATION_STRING_FROM_ALLOWLIST_H
#define QUICPRO_VALIDATION_STRING_FROM_ALLOWLIST_H

#include "php.h"

/**
 * @brief Validates if a zval is a string that exists in a predefined allow-list.
 * @details This function enforces two strict rules:
 * 1. The zval must be of type IS_STRING.
 * 2. The string value must be present in the `allowed_values` array.
 * If either rule is violated, it throws an InvalidArgumentException.
 *
 * @param value The zval to validate.
 * @param allowed_values A NULL-terminated array of strings representing the
 * valid options.
 * @param target A pointer to a char* where the newly allocated, validated
 * string will be stored.
 * @return `SUCCESS` on successful validation, `FAILURE` otherwise.
 */
int qp_validate_string_from_allowlist(zval *value, const char *allowed_values[], char **target);

#endif /* QUICPRO_VALIDATION_STRING_FROM_ALLOWLIST_H */
