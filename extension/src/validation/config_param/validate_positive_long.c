/*
 * =========================================================================
 * FILENAME:   src/validation/config_param/validate_positive_long.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    "0 is just another word for denial." — Every off‑by‑one bug
 *
 * PURPOSE:
 *   Implements qp_validate_positive_long(), ensuring strict type safety
 *   and secure error handling.
 * =========================================================================
 */
#include "include/validation/config_param/validate_positive_long.h"
#include <ext/spl/spl_exceptions.h> /* spl_ce_InvalidArgumentException */

/**
 * @see validate_positive_long.h
 */
int qp_validate_positive_long(zval *value, zend_long *dest)
{
    if (Z_TYPE_P(value) != IS_LONG || Z_LVAL_P(value) <= 0) {
        zend_throw_exception_ex(
            spl_ce_InvalidArgumentException, 0,
            "Invalid value provided. A positive integer greater than zero is required.");
        return FAILURE;
    }

    if (dest) {
        *dest = Z_LVAL_P(value);
    }
    return SUCCESS;
}
