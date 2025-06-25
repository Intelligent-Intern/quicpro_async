/*
 * =========================================================================
 * FILENAME:   include/validation/config_param/validate_niceness_value.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    I am a HAL 9000 computer.
 *
 * PURPOSE:
 * This header declares a centralized, reusable validation helper function
 * for Linux 'niceness' values.
 * =========================================================================
 */

#ifndef QUICPRO_VALIDATION_NICENESS_VALUE_H
#define QUICPRO_VALIDATION_NICENESS_VALUE_H

#include "php.h"

/**
 * @brief Validates if a zval is a valid niceness value (typically -20 to 19).
 * @details This function enforces two strict rules:
 * 1. The zval must be of type IS_LONG.
 * 2. The long value must be within the standard niceness range.
 * If either rule is violated, it throws a detailed InvalidArgumentException.
 *
 * @param value The zval to validate.
 * @param target A pointer to a zend_long where the validated value will be
 * stored upon success.
 * @return `SUCCESS` on successful validation, `FAILURE` otherwise.
 */
int qp_validate_niceness_value(zval *value, zend_long *target);

#endif /* QUICPRO_VALIDATION_NICENESS_VALUE_H */