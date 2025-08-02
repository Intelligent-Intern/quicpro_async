/*
 * =========================================================================
 * FILENAME:   src/config/mcp_and_orchestrator/ini.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Perhaps today is a good day to die!
 *
 * PURPOSE:
 * This file implements the registration and parsing of all `php.ini`
 * settings for the MCP & Orchestrator module.
 * =========================================================================
 */

#include "include/config/mcp_and_orchestrator/ini.h"
#include "include/config/mcp_and_orchestrator/base_layer.h"

#include "php.h"
#include <zend_ini.h>
#include <ext/spl/spl_exceptions.h>

/* Custom OnUpdate handler for positive integer values */
static ZEND_INI_MH(OnUpdateMcpPositiveLong)
{
    zend_long val = zend_atol(ZSTR_VAL(new_value), ZSTR_LEN(new_value));
    if (val <= 0) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Invalid value for an MCP/Orchestrator directive. A positive integer is required.");
        return FAILURE;
    }

    if (zend_string_equals_literal(entry->name, "quicpro.mcp_default_request_timeout_ms")) {
        quicpro_mcp_orchestrator_config.mcp_default_request_timeout_ms = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.mcp_max_message_size_bytes")) {
        quicpro_mcp_orchestrator_config.mcp_max_message_size_bytes = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.mcp_default_retry_max_attempts")) {
        quicpro_mcp_orchestrator_config.mcp_default_retry_max_attempts = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.mcp_default_retry_backoff_ms_initial")) {
        quicpro_mcp_orchestrator_config.mcp_default_retry_backoff_ms_initial = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.mcp_request_cache_ttl_sec")) {
        quicpro_mcp_orchestrator_config.mcp_request_cache_ttl_sec = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.orchestrator_default_pipeline_timeout_ms")) {
        quicpro_mcp_orchestrator_config.orchestrator_default_pipeline_timeout_ms = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.orchestrator_max_recursion_depth")) {
        quicpro_mcp_orchestrator_config.orchestrator_max_recursion_depth = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.orchestrator_loop_concurrency_default")) {
        quicpro_mcp_orchestrator_config.orchestrator_loop_concurrency_default = val;
    }
    return SUCCESS;
}


PHP_INI_BEGIN()
    /* --- MCP Settings --- */
    ZEND_INI_ENTRY_EX("quicpro.mcp_default_request_timeout_ms",      "30000", PHP_INI_SYSTEM, OnUpdateMcpPositiveLong, NULL, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.mcp_max_message_size_bytes",          "4194304", PHP_INI_SYSTEM, OnUpdateMcpPositiveLong, NULL, NULL, NULL)
    STD_PHP_INI_ENTRY("quicpro.mcp_default_retry_policy_enable",     "1",     PHP_INI_SYSTEM, OnUpdateBool, mcp_default_retry_policy_enable, qp_mcp_orchestrator_config_t, quicpro_mcp_orchestrator_config)
    ZEND_INI_ENTRY_EX("quicpro.mcp_default_retry_max_attempts",      "3",     PHP_INI_SYSTEM, OnUpdateMcpPositiveLong, NULL, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.mcp_default_retry_backoff_ms_initial","100",   PHP_INI_SYSTEM, OnUpdateMcpPositiveLong, NULL, NULL, NULL)
    STD_PHP_INI_ENTRY("quicpro.mcp_enable_request_caching",          "0",     PHP_INI_SYSTEM, OnUpdateBool, mcp_enable_request_caching, qp_mcp_orchestrator_config_t, quicpro_mcp_orchestrator_config)
    ZEND_INI_ENTRY_EX("quicpro.mcp_request_cache_ttl_sec",           "60",    PHP_INI_SYSTEM, OnUpdateMcpPositiveLong, NULL, NULL, NULL)

    /* --- Pipeline Orchestrator Settings --- */
    ZEND_INI_ENTRY_EX("quicpro.orchestrator_default_pipeline_timeout_ms", "120000",PHP_INI_SYSTEM, OnUpdateMcpPositiveLong, NULL, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.orchestrator_max_recursion_depth",         "10",    PHP_INI_SYSTEM, OnUpdateMcpPositiveLong, NULL, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.orchestrator_loop_concurrency_default",    "50",    PHP_INI_SYSTEM, OnUpdateMcpPositiveLong, NULL, NULL, NULL)
    STD_PHP_INI_ENTRY("quicpro.orchestrator_enable_distributed_tracing",  "1",     PHP_INI_SYSTEM, OnUpdateBool, orchestrator_enable_distributed_tracing, qp_mcp_orchestrator_config_t, quicpro_mcp_orchestrator_config)
PHP_INI_END()

void qp_config_mcp_and_orchestrator_ini_register(void) { REGISTER_INI_ENTRIES(); }
void qp_config_mcp_and_orchestrator_ini_unregister(void) { UNREGISTER_INI_ENTRIES(); }
