/*
 * =========================================================================
 * FILENAME:   src/config/router_and_loadbalancer/config.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Chevron six encoded.
 *
 * PURPOSE:
 * This file implements the logic for applying runtime configuration
 * changes to the Router & Load Balancer module.
 * =========================================================================
 */

#include "include/config/router_and_loadbalancer/config.h"
#include "include/config/router_and_loadbalancer/base_layer.h"
#include "include/quicpro_globals.h"

#include "include/validation/config_param/validate_bool.h"
#include "include/validation/config_param/validate_positive_long.h"
#include "include/validation/config_param/validate_string_from_allowlist.h"
#include "include/validation/config_param/validate_generic_string.h"

#include "php.h"
#include <ext/spl/spl_exceptions.h>

int qp_config_router_and_loadbalancer_apply_userland_config(zval *config_arr)
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

        if (zend_string_equals_literal(key, "router_mode_enable")) {
            if (qp_validate_bool(value) != SUCCESS) return FAILURE;
            quicpro_router_loadbalancer_config.router_mode_enable = zend_is_true(value);
        } else if (zend_string_equals_literal(key, "hashing_algorithm")) {
            const char *allowed[] = {"consistent_hash", "round_robin", NULL};
            if (qp_validate_string_from_allowlist(value, allowed, &quicpro_router_loadbalancer_config.hashing_algorithm) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "connection_id_entropy_salt")) {
            if (qp_validate_generic_string(value, &quicpro_router_loadbalancer_config.connection_id_entropy_salt) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "backend_discovery_mode")) {
            const char *allowed[] = {"static", "mcp", NULL};
            if (qp_validate_string_from_allowlist(value, allowed, &quicpro_router_loadbalancer_config.backend_discovery_mode) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "backend_static_list")) {
            if (qp_validate_generic_string(value, &quicpro_router_loadbalancer_config.backend_static_list) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "backend_mcp_endpoint")) {
            if (qp_validate_generic_string(value, &quicpro_router_loadbalancer_config.backend_mcp_endpoint) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "backend_mcp_poll_interval_sec")) {
            if (qp_validate_positive_long(value, &quicpro_router_loadbalancer_config.backend_mcp_poll_interval_sec) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "max_forwarding_pps")) {
            if (qp_validate_positive_long(value, &quicpro_router_loadbalancer_config.max_forwarding_pps) != SUCCESS) return FAILURE;
        }

    } ZEND_HASH_FOREACH_END();

    return SUCCESS;
}
