/*
 * =========================================================================
 * FILENAME:   include/validation/config_param/validate_erasure_coding_shards_string.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Fly, you fools!
 *
 * PURPOSE:
 * This header declares a centralized, reusable validation helper function
 * for the erasure coding shards string format (e.g., "8d4p").
 * =========================================================================
 */

#ifndef QUICPRO_VALIDATION_ERASURE_CODING_SHARDS_STRING_H
#define QUICPRO_VALIDATION_ERASURE_CODING_SHARDS_STRING_H

#include "php.h"

/**
 * @brief Validates if a string follows the erasure coding shards format.
 * @details This function enforces a strict format like "XdYp", where X and Y
 * are positive integers representing data and parity shards.
 *
 * @param value The zval to validate.
 * @param target A pointer to a char* where the newly allocated, validated
 * string will be stored.
 * @return `SUCCESS` on successful validation, `FAILURE` otherwise.
 */
int qp_validate_erasure_coding_shards_string(zval *value, char **target);

#endif /* QUICPRO_VALIDATION_ERASURE_CODING_SHARDS_STRING_H */
