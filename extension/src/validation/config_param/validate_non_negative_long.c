/*
 * =========================================================================
 * FILENAME:   src/validation/config_param/validate_non_negative_long.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    No.
 *
 * PURPOSE:
 * This file implements the centralized, reusable validation helper function
 * for non-negative integer values, as declared in its corresponding header file.
 *
 * ARCHITECTURE:
 * The function enforces strict type and range checking. It first verifies
 * that the input is a long, then checks that its value is not negative.
 * This two-step process prevents both type juggling and invalid numeric
 * ranges, providing a robust guard for any configuration parameter that
 * represents a count, size, or other non-negative quantity.
 * =========================================================================
 */

#include "include/validation/config_param/validate_non_negative_long.h"
#include <ext/spl/spl_exceptions.h> /* For spl_ce_InvalidArgumentException */

/**
 * @brief Validates if a zval is a non-negative integer (long).
 * @details This function enforces two strict rules:
 * 1. The zval must be of type IS_LONG.
 * 2. The long value must be zero or greater.
 * If either rule is violated, it throws a detailed InvalidArgumentException.
 * This is crucial for parameters representing counts, sizes, or rates
 * where negative values are nonsensical and dangerous.
 *
 * @param value The zval to validate.
 * @param param_name The name of the configuration parameter being validated,
 * used for generating a clear exception message.
 * @param target A pointer to a zend_long where the validated value will be
 * stored upon success. This prevents the caller from needing to re-extract
 * the value.
 * @return `SUCCESS` on successful validation, `FAILURE` otherwise.
 */
int qp_validate_non_negative_long(zval *value, const char *param_name, zend_long *target)
{
    /* Rule 1: Enforce strict integer type. */
    if (Z_TYPE_P(value) != IS_LONG) {
        zend_throw_exception_ex(
            spl_ce_InvalidArgumentException,
            0,
            "Invalid type for parameter '%s'. An integer is required.",
            param_name
        );
        return FAILURE;
    }

    /* Rule 2: Enforce non-negative value. */
    if (Z_LVAL_P(value) < 0) {
        zend_throw_exception_ex(
            spl_ce_InvalidArgumentException,
            0,
            "Invalid value for parameter '%s'. A non-negative integer is required.",
            param_name
        );
        return FAILURE;
    }

    /* Validation passed, store the value in the target pointer. */
    *target = Z_LVAL_P(value);
    return SUCCESS;
}
