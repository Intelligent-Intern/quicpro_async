/*
 * =========================================================================
 * FILENAME:   include/validation/config_param/validate_host_string.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    It's not personal, Sonny. It's strictly business.
 *
 * PURPOSE:
 * This header declares a centralized, reusable validation helper function
 * for host strings (hostnames or IP addresses).
 * =========================================================================
 */

#ifndef QUICPRO_VALIDATION_HOST_STRING_H
#define QUICPRO_VALIDATION_HOST_STRING_H

#include "php.h"

/**
 * @brief Validates if a zval contains a valid hostname or IP address string.
 * @details This function enforces two strict rules:
 * 1. The zval must be of type IS_STRING.
 * 2. The string must not contain invalid characters for a hostname or IP.
 * This is a basic sanity check and does not perform a DNS lookup.
 *
 * @param value The zval to validate.
 * @param target A pointer to a char* where the newly allocated, validated
 * string will be stored.
 * @return `SUCCESS` on successful validation, `FAILURE` otherwise.
 */
int qp_validate_host_string(zval *value, char **target);

#endif /* QUICPRO_VALIDATION_HOST_STRING_H */
