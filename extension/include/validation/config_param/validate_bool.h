/*
 * =========================================================================
 * FILENAME:   include/validation/config_param/validate_bool.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    I can see it in your eyes. You have the look of a man who accepts what he sees because he is expecting to wake up.
 *
 * PURPOSE:
 * This header declares a centralized, reusable validation helper function
 * for boolean values passed from PHP userland.
 * =========================================================================
 */

#ifndef QUICPRO_VALIDATION_BOOL_H
#define QUICPRO_VALIDATION_BOOL_H

#include "php.h"

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
int qp_validate_bool(zval *value, const char *param_name);

#endif /* QUICPRO_VALIDATION_BOOL_H */
