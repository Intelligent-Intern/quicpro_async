/*
 * =========================================================================
 * FILENAME:   src/validation/config_param/validate_erasure_coding_shards_string.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    A wizard is never late, Frodo Baggins. Nor is he early.
 * He arrives precisely when he means to.
 *
 * PURPOSE:
 * This file implements the validation helper for erasure coding shard strings.
 *
 * ARCHITECTURE:
 * The function uses `sscanf` to parse the string according to the strict
 * "XdYp" format, ensuring both the structure and the numeric values of
 * the data and parity shards are valid before accepting the configuration.
 * =========================================================================
 */

#include "include/validation/config_param/validate_erasure_coding_shards_string.h"
#include <ext/spl/spl_exceptions.h>
#include <stdio.h>

int qp_validate_erasure_coding_shards_string(zval *value, char **target) {
    if (Z_TYPE_P(value) != IS_STRING) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Invalid type for erasure coding shards. A string is required.");
        return FAILURE;
    }

    char *input_str = Z_STRVAL_P(value);
    int data_shards, parity_shards;
    char d, p;

    /* Use sscanf to strictly match the "integer, char, integer, char" format */
    if (sscanf(input_str, "%d%c%d%c", &data_shards, &d, &parity_shards, &p) != 4 ||
        d != 'd' || p != 'p' || data_shards <= 0 || parity_shards <= 0)
    {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Invalid format for erasure coding shards. Expected format like '8d4p' with positive integers.");
        return FAILURE;
    }

    if (*target) {
        pefree(*target, 1);
    }
    *target = pestrdup(input_str, 1);
    return SUCCESS;
}
