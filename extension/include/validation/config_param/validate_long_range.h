/*
 * =========================================================================
 * FILENAME:   include/validation/config_param/validate_long_range.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    I will face my fear. I will permit it to pass over me and
 * through me.
 *
 * PURPOSE:
 * This header declares a validation helper for integer values that must
 * be within a specific range.
 * =========================================================================
 */

#ifndef QUICPRO_VALIDATION_LONG_RANGE_H
#define QUICPRO_VALIDATION_LONG_RANGE_H

#include "php.h"

/**
 * @brief Validates if a zval is a long within a specified [min, max] range.
 * @param value The zval to validate.
 * @param min The minimum allowed value (inclusive).
 * @param max The maximum allowed value (inclusive).
 * @param target A pointer to a zend_long where the validated value will be stored.
 * @return `SUCCESS` on successful validation, `FAILURE` otherwise.
 */
int qp_validate_long_range(zval *value, zend_long min, zend_long max, zend_long *target);

#endif /* QUICPRO_VALIDATION_LONG_RANGE_H */
