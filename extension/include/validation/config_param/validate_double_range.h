/*
 * =========================================================================
 * FILENAME:   include/validation/config_param/validate_double_range.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Remember: all I'm offering is the truth. Nothing more.
 *
 * PURPOSE:
 * This header declares a centralized, reusable validation helper function
 * for floating-point values that must be within a specific range.
 * =========================================================================
 */

#ifndef QUICPRO_VALIDATION_DOUBLE_RANGE_H
#define QUICPRO_VALIDATION_DOUBLE_RANGE_H

#include "php.h"

/**
 * @brief Validates if a zval is a double within a specified [min, max] range.
 * @details This function enforces two strict rules:
 * 1. The zval must be of type IS_DOUBLE.
 * 2. The double value must be >= min and <= max.
 *
 * @param value The zval to validate.
 * @param min The minimum allowed value (inclusive).
 * @param max The maximum allowed value (inclusive).
 * @param target A pointer to a double where the validated value will be stored.
 * @return `SUCCESS` on successful validation, `FAILURE` otherwise.
 */
int qp_validate_double_range(zval *value, double min, double max, double *target);

#endif /* QUICPRO_VALIDATION_DOUBLE_RANGE_H */
