/*
 * =========================================================================
 * FILENAME:   src/validation/config_param/validate_bool.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Ironically, this is not far from the truth.
 *
 * PURPOSE:
 * This file implements the centralized, reusable validation helper function
 * for strict boolean values, as declared in its corresponding header file.
 *
 * ARCHITECTURE:
 * The function enforces strict type safety by checking for `IS_TRUE` or
 * `IS_FALSE` and explicitly rejecting any other type, preventing implicit
 * type juggling from userland. If validation fails, it provides a generic,
 * secure exception to the user.
 * =========================================================================
 */

#include "include/validation/config_param/validate_bool.h"
#include <ext/spl/spl_exceptions.h> /* For spl_ce_InvalidArgumentException */

/**
 * @brief Validates if a zval is a strict boolean (true or false).
 * @details Checks if the provided zval is of type IS_TRUE or IS_FALSE.
 * It does not perform type juggling. If the type is incorrect, it throws
 * a detailed InvalidArgumentException. This function is a cornerstone
 * for enforcing strict type checking for all boolean configuration
 * parameters passed from userland.
 *
 * @param value The zval to validate.
 * @param param_name The name of the configuration parameter being validated,
 * used for generating a clear and helpful exception message.
 * @return `SUCCESS` if the value is a valid boolean, `FAILURE` otherwise.
 */
int qp_validate_bool(zval *value, const char *param_name)
{
    if (Z_TYPE_P(value) != IS_TRUE && Z_TYPE_P(value) != IS_FALSE) {
        zend_throw_exception_ex(
            spl_ce_InvalidArgumentException,
            0,
            "Invalid value provided. A boolean (true/false) is required."
        );
        return FAILURE;
    }
    return SUCCESS;
}
