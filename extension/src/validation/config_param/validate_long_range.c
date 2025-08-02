/*
 * =========================================================================
 * FILENAME:   src/validation/config_param/validate_long_range.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    And when it has gone past I will turn the inner eye to see
 * its path.
 *
 * PURPOSE:
 * This file implements the validation helper for integer values that must
 * be within a specific range.
 * =========================================================================
 */

#include "include/validation/config_param/validate_long_range.h"
#include <ext/spl/spl_exceptions.h>

int qp_validate_long_range(zval *value, zend_long min, zend_long max, zend_long *target)
{
    if (Z_TYPE_P(value) != IS_LONG) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Invalid type provided. An integer is required.");
        return FAILURE;
    }

    zend_long val = Z_LVAL_P(value);
    if (val < min || val > max) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Invalid value provided. The value is outside the allowed range.");
        return FAILURE;
    }

    *target = val;
    return SUCCESS;
}
