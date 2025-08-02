/*
 * =========================================================================
 * FILENAME:   include/validation/config_param/validate_comma_separated_numeric_string.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    What good is a phone call if you are unable to speak?
 *
 * PURPOSE:
 * This header declares a validation helper for comma-separated string
 * values where each token must be a valid number.
 * =========================================================================
 */

#ifndef QUICPRO_VALIDATION_COMMA_SEPARATED_NUMERIC_STRING_H
#define QUICPRO_VALIDATION_COMMA_SEPARATED_NUMERIC_STRING_H

#include "php.h"

/**
 * @brief Validates if a zval is a comma-separated string of numeric values.
 *
 * @param value The zval to validate.
 * @param target A pointer to a char* where the newly allocated, validated
 * string will be stored.
 * @return `SUCCESS` on successful validation, `FAILURE` otherwise.
 */
int qp_validate_comma_separated_numeric_string(zval *value, char **target);

#endif /* QUICPRO_VALIDATION_COMMA_SEPARATED_NUMERIC_STRING_H */