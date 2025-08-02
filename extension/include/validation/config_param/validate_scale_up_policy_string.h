/*
 * =========================================================================
 * FILENAME:   include/validation/config_param/validate_scale_up_policy_string.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Where the fear has gone there will be nothing.
 *
 * PURPOSE:
 * This header declares a validation helper for the scale-up policy string.
 * =========================================================================
 */

#ifndef QUICPRO_VALIDATION_SCALE_UP_POLICY_STRING_H
#define QUICPRO_VALIDATION_SCALE_UP_POLICY_STRING_H

#include "php.h"

/**
 * @brief Validates if a string follows the scale-up policy format.
 * @details This function enforces a strict format like "action:value".
 *
 * @param value The zval to validate.
 * @param target A pointer to a char* where the newly allocated string will be stored.
 * @return `SUCCESS` on successful validation, `FAILURE` otherwise.
 */
int qp_validate_scale_up_policy_string(zval *value, char **target);

#endif /* QUICPRO_VALIDATION_SCALE_UP_POLICY_STRING_H */