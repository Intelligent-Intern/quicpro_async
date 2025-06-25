/*
 * =========================================================================
 * FILENAME:   src/config/bare_metal_tuning/ini.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    The light that burns twice as bright burns half as long -
 * and you have burned so very, very brightly, Roy.
 *
 * PURPOSE:
 * This file implements the registration, parsing, and validation of all
 * `php.ini` settings for the bare metal tuning module.
 * =========================================================================
 */

#include "include/config/bare_metal_tuning/ini.h"
#include "include/config/bare_metal_tuning/base_layer.h"
#include "include/validation/config_param/validate_cpu_affinity_map_string.h"

#include "php.h"
#include <zend_ini.h>
#include <ext/spl/spl_exceptions.h>

static ZEND_INI_MH(OnUpdateBareMetalNonNegativeLong)
{
    zend_long val = zend_atol(ZSTR_VAL(new_value), ZSTR_LEN(new_value));
    if (val < 0) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Invalid value provided for a bare-metal directive. A non-negative integer is required.");
        return FAILURE;
    }
    if (zend_string_equals_literal(entry->name, "quicpro.io_uring_sq_poll_ms")) {
        quicpro_bare_metal_config.io_uring_sq_poll_ms = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.socket_enable_busy_poll_us")) {
        quicpro_bare_metal_config.socket_enable_busy_poll_us = val;
    }
    return SUCCESS;
}

static ZEND_INI_MH(OnUpdateBareMetalPositiveLong)
{
    zend_long val = zend_atol(ZSTR_VAL(new_value), ZSTR_LEN(new_value));
    if (val <= 0) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Invalid value provided for a bare-metal directive. A positive integer (greater than zero) is required.");
        return FAILURE;
    }
    if (zend_string_equals_literal(entry->name, "quicpro.io_max_batch_read_packets")) {
        quicpro_bare_metal_config.io_max_batch_read_packets = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.io_max_batch_write_packets")) {
        quicpro_bare_metal_config.io_max_batch_write_packets = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.socket_receive_buffer_size")) {
        quicpro_bare_metal_config.socket_receive_buffer_size = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.socket_send_buffer_size")) {
        quicpro_bare_metal_config.socket_send_buffer_size = val;
    }
    return SUCCESS;
}

static ZEND_INI_MH(OnUpdateNumaPolicyString)
{
    const char *policy = ZSTR_VAL(new_value);
    if (strcasecmp(policy, "default") != 0 && strcasecmp(policy, "prefer") != 0 && strcasecmp(policy, "bind") != 0 && strcasecmp(policy, "interleave") != 0) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Invalid value for NUMA policy. Must be one of 'default', 'prefer', 'bind', or 'interleave'.");
        return FAILURE;
    }
    OnUpdateString(entry, new_value, mh_arg1, mh_arg2, mh_arg3);
    return SUCCESS;
}

static ZEND_INI_MH(OnUpdateCpuAffinityString)
{
    if (qp_validate_cpu_affinity_map_string(new_value, &quicpro_bare_metal_config.io_thread_cpu_affinity) != SUCCESS) {
        return FAILURE;
    }
    return SUCCESS;
}

PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("quicpro.io_engine_use_uring", "1", PHP_INI_SYSTEM, OnUpdateBool, io_engine_use_uring, qp_bare_metal_config_t, quicpro_bare_metal_config)
    ZEND_INI_ENTRY_EX("quicpro.io_uring_sq_poll_ms", "0", PHP_INI_SYSTEM, OnUpdateBareMetalNonNegativeLong, NULL, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.io_max_batch_read_packets","64", PHP_INI_SYSTEM, OnUpdateBareMetalPositiveLong, NULL, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.io_max_batch_write_packets","64", PHP_INI_SYSTEM, OnUpdateBareMetalPositiveLong, NULL, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.socket_receive_buffer_size", "2097152", PHP_INI_SYSTEM, OnUpdateBareMetalPositiveLong, NULL, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.socket_send_buffer_size", "2097152", PHP_INI_SYSTEM, OnUpdateBareMetalPositiveLong, NULL, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.socket_enable_busy_poll_us", "0", PHP_INI_SYSTEM, OnUpdateBareMetalNonNegativeLong, NULL, NULL, NULL)
    STD_PHP_INI_ENTRY("quicpro.socket_enable_timestamping", "1", PHP_INI_SYSTEM, OnUpdateBool, socket_enable_timestamping, qp_bare_metal_config_t, quicpro_bare_metal_config)
    ZEND_INI_ENTRY_EX("quicpro.io_thread_cpu_affinity", "", PHP_INI_SYSTEM, OnUpdateCpuAffinityString, NULL, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.io_thread_numa_node_policy", "default", PHP_INI_SYSTEM, OnUpdateNumaPolicyString, &quicpro_bare_metal_config.io_thread_numa_node_policy, NULL, NULL)
PHP_INI_END()

void qp_config_bare_metal_tuning_ini_register(void)
{
    REGISTER_INI_ENTRIES();
}

void qp_config_bare_metal_tuning_ini_unregister(void)
{
    UNREGISTER_INI_ENTRIES();
}
