/*
 * =========================================================================
 * FILENAME:   include/validation/config_param/validate_positive_long.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Positive vibes only — and integers greater than zero.
 *
 * PURPOSE:
 *   Declares a reusable helper to validate that a zval contains a strictly
 *   positive ( > 0 ) long value.  Used by numerous runtime configuration
 *   paths where negative, zero or non‑integer values would be unsafe.
 * =========================================================================
 */

#ifndef QUICPRO_VALIDATION_POSITIVE_LONG_H
#define QUICPRO_VALIDATION_POSITIVE_LONG_H

#include "php.h"

/**
 * @brief Validates that a zval is a strictly‑positive long and copies it.
 *
 * If the check fails, an \c InvalidArgumentException is thrown and
 * \c FAILURE is returned.
 *
 * @param value  The zval to validate (expected IS_LONG).
 * @param dest   Pointer where the validated value will be copied to.
 * @return       \c SUCCESS on success, \c FAILURE otherwise.
 */
int qp_validate_positive_long(zval *value, zend_long *dest);

#endif /* QUICPRO_VALIDATION_POSITIVE_LONG_H */
