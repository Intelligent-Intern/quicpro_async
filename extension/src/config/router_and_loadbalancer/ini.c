/*
 * =========================================================================
 * FILENAME:   src/config/router_and_loadbalancer/ini.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Chevron five encoded.
 *
 * PURPOSE:
 * This file implements the registration, parsing, and validation of all
 * `php.ini` settings for the Router & Load Balancer module.
 * =========================================================================
 */

#include "include/config/router_and_loadbalancer/ini.h"
#include "include/config/router_and_loadbalancer/base_layer.h"
#include "php.h"
#include <zend_ini.h>
#include <ext/spl/spl_exceptions.h>

static ZEND_INI_MH(OnUpdateRouterPositiveLong)
{
    zend_long val = zend_atol(ZSTR_VAL(new_value), ZSTR_LEN(new_value));
    if (val <= 0) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Invalid value for a router directive. A positive integer is required.");
        return FAILURE;
    }
    if (zend_string_equals_literal(entry->name, "quicpro.router_backend_mcp_poll_interval_sec")) {
        quicpro_router_loadbalancer_config.backend_mcp_poll_interval_sec = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.router_max_forwarding_pps")) {
        quicpro_router_loadbalancer_config.max_forwarding_pps = val;
    }
    return SUCCESS;
}

static ZEND_INI_MH(OnUpdateRouterAllowlist)
{
    const char *hashing_allowed[] = {"consistent_hash", "round_robin", NULL};
    const char *discovery_allowed[] = {"static", "mcp", NULL};
    const char **current_list = NULL;

    if (zend_string_equals_literal(entry->name, "quicpro.router_hashing_algorithm")) {
        current_list = hashing_allowed;
    } else if (zend_string_equals_literal(entry->name, "quicpro.router_backend_discovery_mode")) {
        current_list = discovery_allowed;
    }

    bool is_allowed = false;
    for (int i = 0; current_list[i] != NULL; i++) {
        if (strcasecmp(ZSTR_VAL(new_value), current_list[i]) == 0) {
            is_allowed = true;
            break;
        }
    }
    if (!is_allowed) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Invalid value provided for a router directive. The value is not one of the allowed options.");
        return FAILURE;
    }
    OnUpdateString(entry, new_value, mh_arg1, mh_arg2, mh_arg3);
    return SUCCESS;
}


PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("quicpro.router_mode_enable", "0", PHP_INI_SYSTEM, OnUpdateBool, router_mode_enable, qp_router_loadbalancer_config_t, quicpro_router_loadbalancer_config)
    ZEND_INI_ENTRY_EX("quicpro.router_hashing_algorithm", "consistent_hash", PHP_INI_SYSTEM, OnUpdateRouterAllowlist, &quicpro_router_loadbalancer_config.hashing_algorithm, NULL, NULL)
    STD_PHP_INI_ENTRY("quicpro.router_connection_id_entropy_salt", "change-this-to-a-long-random-string-in-production", PHP_INI_SYSTEM, OnUpdateString, connection_id_entropy_salt, qp_router_loadbalancer_config_t, quicpro_router_loadbalancer_config)
    ZEND_INI_ENTRY_EX("quicpro.router_backend_discovery_mode", "static", PHP_INI_SYSTEM, OnUpdateRouterAllowlist, &quicpro_router_loadbalancer_config.backend_discovery_mode, NULL, NULL)
    STD_PHP_INI_ENTRY("quicpro.router_backend_static_list", "127.0.0.1:8443", PHP_INI_SYSTEM, OnUpdateString, backend_static_list, qp_router_loadbalancer_config_t, quicpro_router_loadbalancer_config)
    STD_PHP_INI_ENTRY("quicpro.router_backend_mcp_endpoint", "127.0.0.1:9998", PHP_INI_SYSTEM, OnUpdateString, backend_mcp_endpoint, qp_router_loadbalancer_config_t, quicpro_router_loadbalancer_config)
    ZEND_INI_ENTRY_EX("quicpro.router_backend_mcp_poll_interval_sec", "10", PHP_INI_SYSTEM, OnUpdateRouterPositiveLong, NULL, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.router_max_forwarding_pps", "1000000", PHP_INI_SYSTEM, OnUpdateRouterPositiveLong, NULL, NULL, NULL)
PHP_INI_END()

void qp_config_router_and_loadbalancer_ini_register(void) { REGISTER_INI_ENTRIES(); }
void qp_config_router_and_loadbalancer_ini_unregister(void) { UNREGISTER_INI_ENTRIES(); }
