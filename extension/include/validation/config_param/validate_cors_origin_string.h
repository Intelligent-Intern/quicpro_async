/*
 * =========================================================================
 * FILENAME:   include/validation/config_param/validate_cors_origin_string.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Why not?
 *
 * PURPOSE:
 * This header declares a centralized, reusable validation helper function
 * for CORS (Cross-Origin Resource Sharing) origin strings passed from
 * PHP userland.
 * =========================================================================
 */

#ifndef QUICPRO_VALIDATION_CORS_ORIGIN_STRING_H
#define QUICPRO_VALIDATION_CORS_ORIGIN_STRING_H

#include "php.h"

/**
 * @brief Validates if a zval contains a valid CORS origin string.
 * @details This function provides robust validation for the CORS policy string.
 * It accepts either a single wildcard `"*"` or a comma-separated list of
 * valid origins. Each origin in the list is individually parsed using
 * PHP's internal `php_url_parse_ex` function to ensure it has a valid
 * structure (`scheme://host:port`) and an appropriate scheme (`http` or
 * `https`).
 *
 * @param value The zval to validate, which must be a string.
 * @param param_name The name of the configuration parameter being validated.
 * @param target A pointer to a char* where a newly allocated, validated
 * string will be stored. The caller is responsible for managing the
 * memory of any pre-existing string in the target.
 * @return `SUCCESS` on successful validation, `FAILURE` otherwise.
 */
int qp_validate_cors_origin_string(zval *value, const char *param_name, char **target);

#endif /* QUICPRO_VALIDATION_CORS_ORIGIN_STRING_H */
