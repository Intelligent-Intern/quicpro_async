/*
 * =========================================================================
 * FILENAME:   src/config/iibin/ini.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    The answer to the ultimate question of life, the universe
 * and everything is 42.
 *
 * PURPOSE:
 * This file implements the registration and parsing of all `php.ini`
 * settings for the IIBIN serialization module.
 * =========================================================================
 */

#include "include/config/iibin/ini.h"
#include "include/config/iibin/base_layer.h"

#include "php.h"
#include <zend_ini.h>
#include <ext/spl/spl_exceptions.h>

/* Custom OnUpdate handler for positive integer values */
static ZEND_INI_MH(OnUpdateIibinPositiveLong)
{
    zend_long val = zend_atol(ZSTR_VAL(new_value), ZSTR_LEN(new_value));
    if (val <= 0) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Invalid value for an IIBIN directive. A positive integer is required.");
        return FAILURE;
    }

    if (zend_string_equals_literal(entry->name, "quicpro.iibin_max_schema_fields")) {
        quicpro_iibin_config.max_schema_fields = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.iibin_max_recursion_depth")) {
        quicpro_iibin_config.max_recursion_depth = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.io_default_buffer_size_kb")) {
        quicpro_iibin_config.default_buffer_size_kb = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.io_shm_total_memory_mb")) {
        quicpro_iibin_config.shm_total_memory_mb = val;
    }
    return SUCCESS;
}


PHP_INI_BEGIN()
    /* --- IIBIN Serialization Engine --- */
    ZEND_INI_ENTRY_EX("quicpro.iibin_max_schema_fields", "256", PHP_INI_SYSTEM, OnUpdateIibinPositiveLong, NULL, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.iibin_max_recursion_depth", "32", PHP_INI_SYSTEM, OnUpdateIibinPositiveLong, NULL, NULL, NULL)
    STD_PHP_INI_ENTRY("quicpro.iibin_string_interning_enable", "1", PHP_INI_SYSTEM, OnUpdateBool, string_interning_enable, qp_iibin_config_t, quicpro_iibin_config)

    /* --- Zero-Copy I/O via Shared Memory Buffers --- */
    STD_PHP_INI_ENTRY("quicpro.io_use_shared_memory_buffers", "0", PHP_INI_SYSTEM, OnUpdateBool, use_shared_memory_buffers, qp_iibin_config_t, quicpro_iibin_config)
    ZEND_INI_ENTRY_EX("quicpro.io_default_buffer_size_kb", "64", PHP_INI_SYSTEM, OnUpdateIibinPositiveLong, NULL, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.io_shm_total_memory_mb", "256", PHP_INI_SYSTEM, OnUpdateIibinPositiveLong, NULL, NULL, NULL)
    STD_PHP_INI_ENTRY("quicpro.io_shm_path", "/quicpro_io_shm", PHP_INI_SYSTEM, OnUpdateString, shm_path, qp_iibin_config_t, quicpro_iibin_config)
PHP_INI_END()

void qp_config_iibin_ini_register(void) { REGISTER_INI_ENTRIES(); }
void qp_config_iibin_ini_unregister(void) { UNREGISTER_INI_ENTRIES(); }
