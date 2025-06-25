/*
 * =========================================================================
 * FILENAME:   include/validation/config_param/validate_generic_string.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    It's called "Daisy."
 *
 * PURPOSE:
 * This header declares a simple, reusable validation helper function
 * for generic, non-empty string parameters.
 * =========================================================================
 */

#ifndef QUICPRO_VALIDATION_GENERIC_STRING_H
#define QUICPRO_VALIDATION_GENERIC_STRING_H

#include "php.h"

/**
 * @brief Validates if a zval is a string (can be empty).
 *
 * @param value The zval to validate.
 * @param target A pointer to a char* where the newly allocated string
 * will be stored.
 * @return `SUCCESS` on successful validation, `FAILURE` otherwise.
 */
int qp_validate_generic_string(zval *value, char **target);

#endif /* QUICPRO_VALIDATION_GENERIC_STRING_H */