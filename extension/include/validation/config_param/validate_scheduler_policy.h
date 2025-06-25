/*
 * =========================================================================
 * FILENAME:   include/validation/config_param/validate_scheduler_policy.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    My instructor was Mr. Langley, and he taught me to sing a song.
 *
 * PURPOSE:
 * This header declares a centralized, reusable validation helper function
 * for Linux scheduler policy strings.
 * =========================================================================
 */

#ifndef QUICPRO_VALIDATION_SCHEDULER_POLICY_H
#define QUICPRO_VALIDATION_SCHEDULER_POLICY_H

#include "php.h"

/**
 * @brief Validates if a zval is a valid scheduler policy string.
 * @details This function enforces two strict rules:
 * 1. The zval must be of type IS_STRING.
 * 2. The string value must be one of the allowed policies ("other", "fifo", "rr").
 *
 * @param value The zval to validate.
 * @param target A pointer to a char* where the newly allocated, validated
 * string will be stored.
 * @return `SUCCESS` on successful validation, `FAILURE` otherwise.
 */
int qp_validate_scheduler_policy(zval *value, char **target);

#endif /* QUICPRO_VALIDATION_SCHEDULER_POLICY_H */