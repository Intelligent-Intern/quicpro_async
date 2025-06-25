/*
 * =========================================================================
 * FILENAME:   src/validation/config_param/validate_generic_string.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Daisy, Daisy, give me your answer do...
 *
 * PURPOSE:
 * This file implements a simple validation helper for generic strings.
 *
 * ARCHITECTURE:
 * This function serves as a basic type check for parameters that accept
 * any string content, including an empty string. Its main purpose is to
 * ensure type safety and provide a consistent validation pattern.
 * =========================================================================
 */

#include "include/validation/config_param/validate_generic_string.h"
#include <ext/spl/spl_exceptions.h> /* For spl_ce_InvalidArgumentException */

/**
 * @brief Validates if a zval is a string (can be empty).
 */
int qp_validate_generic_string(zval *value, char **target)
{
    if (Z_TYPE_P(value) != IS_STRING) {
        zend_throw_exception_ex(
            spl_ce_InvalidArgumentException,
            0,
            "Invalid type provided. A string is required."
        );
        return FAILURE;
    }

    /* Free the old string if it exists. */
    if (*target) {
        pefree(*target, 1);
    }
    *target = pestrdup(Z_STRVAL_P(value), 1);

    return SUCCESS;
}
