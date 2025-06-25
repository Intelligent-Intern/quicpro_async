/*
 * =========================================================================
 * FILENAME:   include/validation/config_param/validate_cpu_affinity_map_string.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Commerce is our goal here at Tyrell. 'More human than
 * human' is our motto.
 *
 * PURPOSE:
 * This header declares a centralized, reusable validation helper function
 * for the complex CPU affinity map string format.
 * =========================================================================
 */

#ifndef QUICPRO_VALIDATION_CPU_AFFINITY_MAP_STRING_H
#define QUICPRO_VALIDATION_CPU_AFFINITY_MAP_STRING_H

#include "php.h"

/**
 * @brief Validates if a string follows the CPU affinity map format.
 * @details This function enforces a strict format like "0:0-1,1:2-3". It ensures:
 * 1. The input is a string.
 * 2. It's a comma-separated list of entries.
 * 3. Each entry is in the format `worker_id:core_id` or `worker_id:core_start-core_end`.
 * 4. All IDs and ranges are valid non-negative integers.
 *
 * @param value The zval to validate.
 * @param target A pointer to a char* where the newly allocated, validated
 * string will be stored.
 * @return `SUCCESS` on successful validation, `FAILURE` otherwise.
 */
int qp_validate_cpu_affinity_map_string(zval *value, char **target);

#endif /* QUICPRO_VALIDATION_CPU_AFFINITY_MAP_STRING_H */
