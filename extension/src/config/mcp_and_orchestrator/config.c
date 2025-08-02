/*
 * =========================================================================
 * FILENAME:   src/config/mcp_and_orchestrator/config.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Resistance is futile.
 *
 * PURPOSE:
 * This file implements the logic for applying runtime configuration
 * changes to the MCP & Orchestrator module.
 * =========================================================================
 */

#include "include/config/mcp_and_orchestrator/config.h"
#include "include/config/mcp_and_orchestrator/base_layer.h"
#include "include/quicpro_globals.h"

/* Centralized validation helpers */
#include "include/validation/config_param/validate_bool.h"
#include "include/validation/config_param/validate_positive_long.h"

#include "php.h"
#include <ext/spl/spl_exceptions.h>

int qp_config_mcp_and_orchestrator_apply_userland_config(zval *config_arr)
{
    zval *value;
    zend_string *key;

    if (!quicpro_globals.is_userland_override_allowed) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Configuration override from userland is disabled by system administrator.");
        return FAILURE;
    }

    if (Z_TYPE_P(config_arr) != IS_ARRAY) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Configuration must be provided as an array.");
        return FAILURE;
    }

    ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(config_arr), key, value) {
        if (!key) continue;

        if (zend_string_equals_literal(key, "mcp_default_request_timeout_ms")) {
            if (qp_validate_positive_long(value, &quicpro_mcp_orchestrator_config.mcp_default_request_timeout_ms) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "mcp_max_message_size_bytes")) {
            if (qp_validate_positive_long(value, &quicpro_mcp_orchestrator_config.mcp_max_message_size_bytes) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "mcp_default_retry_policy_enable")) {
            if (qp_validate_bool(value) != SUCCESS) return FAILURE;
            quicpro_mcp_orchestrator_config.mcp_default_retry_policy_enable = zend_is_true(value);
        } else if (zend_string_equals_literal(key, "mcp_default_retry_max_attempts")) {
            if (qp_validate_positive_long(value, &quicpro_mcp_orchestrator_config.mcp_default_retry_max_attempts) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "mcp_default_retry_backoff_ms_initial")) {
            if (qp_validate_positive_long(value, &quicpro_mcp_orchestrator_config.mcp_default_retry_backoff_ms_initial) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "mcp_enable_request_caching")) {
            if (qp_validate_bool(value) != SUCCESS) return FAILURE;
            quicpro_mcp_orchestrator_config.mcp_enable_request_caching = zend_is_true(value);
        } else if (zend_string_equals_literal(key, "mcp_request_cache_ttl_sec")) {
            if (qp_validate_positive_long(value, &quicpro_mcp_orchestrator_config.mcp_request_cache_ttl_sec) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "orchestrator_default_pipeline_timeout_ms")) {
            if (qp_validate_positive_long(value, &quicpro_mcp_orchestrator_config.orchestrator_default_pipeline_timeout_ms) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "orchestrator_max_recursion_depth")) {
            if (qp_validate_positive_long(value, &quicpro_mcp_orchestrator_config.orchestrator_max_recursion_depth) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "orchestrator_loop_concurrency_default")) {
            if (qp_validate_positive_long(value, &quicpro_mcp_orchestrator_config.orchestrator_loop_concurrency_default) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "orchestrator_enable_distributed_tracing")) {
            if (qp_validate_bool(value) != SUCCESS) return FAILURE;
            quicpro_mcp_orchestrator_config.orchestrator_enable_distributed_tracing = zend_is_true(value);
        }

    } ZEND_HASH_FOREACH_END();

    return SUCCESS;
}
