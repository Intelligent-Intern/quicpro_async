/*
 * =========================================================================
 * FILENAME:   include/validation/config_param/validate_readable_file_path.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    My father taught me many things here... He taught me:
 * keep your friends close, but your enemies closer.
 *
 * PURPOSE:
 * This header declares a validation helper that checks if a file path
 * exists and is readable.
 * =========================================================================
 */

#ifndef QUICPRO_VALIDATION_READABLE_FILE_PATH_H
#define QUICPRO_VALIDATION_READABLE_FILE_PATH_H

#include "php.h"

/**
 * @brief Validates if a zval contains a string that is a readable file path.
 * @details This function enforces three strict rules:
 * 1. The zval must be of type IS_STRING.
 * 2. The string must not be empty.
 * 3. The path must point to a regular file that exists and is readable.
 *
 * @param value The zval to validate.
 * @param target A pointer to a char* where the newly allocated, validated
 * path will be stored.
 * @return `SUCCESS` on successful validation, `FAILURE` otherwise.
 */
int qp_validate_readable_file_path(zval *value, char **target);

#endif /* QUICPRO_VALIDATION_READABLE_FILE_PATH_H */
