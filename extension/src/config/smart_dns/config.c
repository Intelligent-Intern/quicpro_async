/*
 * =========================================================================
 * FILENAME:   src/config/smart_dns/config.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    "Configure wisely – the root servers are watching."
 *
 * PURPOSE:
 * Implements the logic to apply validated, runtime configuration overrides
 * coming from PHP userland (Quicpro\Config object) to the Smart-DNS module.
 * =========================================================================
 */

#include "include/config/smart_dns/config.h"
#include "include/config/smart_dns/base_layer.h"
#include "include/quicpro_globals.h"

/* Centralised validation helpers */
#include "include/validation/config_param/validate_bool.h"
#include "include/validation/config_param/validate_positive_long.h"
#include "include/validation/config_param/validate_string_from_allowlist.h"

#include "php.h"
#include <ext/spl/spl_exceptions.h>

int qp_config_smart_dns_apply_userland_config(zval *config_arr)
{
    zval *value;
    zend_string *key;

    /* Step 0: Enforce global security policy */
    if (!quicpro_globals.is_userland_override_allowed) {
        zend_throw_exception_ex(
            spl_ce_InvalidArgumentException, 0,
            "Configuration override from userland is disabled by system administrator.");
        return FAILURE;
    }

    if (Z_TYPE_P(config_arr) != IS_ARRAY) {
        zend_throw_exception_ex(
            spl_ce_InvalidArgumentException, 0,
            "Configuration must be provided as an array.");
        return FAILURE;
    }

    ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(config_arr), key, value) {
        if (!key) {
            continue;
        }

        /* Boolean settings */
        if (zend_string_equals_literal(key, "dns_server_enable")) {
            if (qp_validate_bool(value) == SUCCESS) {
                quicpro_smart_dns_config.dns_server_enable = zend_is_true(value);
            } else { return FAILURE; }
        } else if (zend_string_equals_literal(key, "dns_server_enable_tcp")) {
            if (qp_validate_bool(value) == SUCCESS) {
                quicpro_smart_dns_config.dns_server_enable_tcp = zend_is_true(value);
            } else { return FAILURE; }
        } else if (zend_string_equals_literal(key, "dns_enable_dnssec_validation")) {
            if (qp_validate_bool(value) == SUCCESS) {
                quicpro_smart_dns_config.dns_enable_dnssec_validation = zend_is_true(value);
            } else { return FAILURE; }
        } else if (zend_string_equals_literal(key, "dns_semantic_mode_enable")) {
            if (qp_validate_bool(value) == SUCCESS) {
                quicpro_smart_dns_config.dns_semantic_mode_enable = zend_is_true(value);
            } else { return FAILURE; }
        /* Positive integer settings */
        } else if (zend_string_equals_literal(key, "dns_server_port")) {
            if (qp_validate_positive_long(value, &quicpro_smart_dns_config.dns_server_port) != SUCCESS) {
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "dns_default_record_ttl_sec")) {
            if (qp_validate_positive_long(value, &quicpro_smart_dns_config.dns_default_record_ttl_sec) != SUCCESS) {
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "dns_service_discovery_max_ips_per_response")) {
            if (qp_validate_positive_long(value, &quicpro_smart_dns_config.dns_service_discovery_max_ips_per_response) != SUCCESS) {
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "dns_edns_udp_payload_size")) {
            if (qp_validate_positive_long(value, &quicpro_smart_dns_config.dns_edns_udp_payload_size) != SUCCESS) {
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "dns_mothernode_sync_interval_sec")) {
            if (qp_validate_positive_long(value, &quicpro_smart_dns_config.dns_mothernode_sync_interval_sec) != SUCCESS) {
                return FAILURE;
            }
        /* Enumerated string settings */
        } else if (zend_string_equals_literal(key, "dns_mode")) {
            const char *allowed[] = {"authoritative", "recursive_resolver", "service_discovery", NULL};
            if (qp_validate_string_from_allowlist(value, allowed, &quicpro_smart_dns_config.dns_mode) != SUCCESS) {
                return FAILURE;
            }
        /* Generic string settings */
        } else if (zend_string_equals_literal(key, "dns_server_bind_host")) {
            if (Z_TYPE_P(value) == IS_STRING) {
                quicpro_smart_dns_config.dns_server_bind_host = estrdup(Z_STRVAL_P(value));
            } else {
                zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0,
                    "dns_server_bind_host must be a string.");
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "dns_static_zone_file_path")) {
            if (Z_TYPE_P(value) == IS_STRING) {
                quicpro_smart_dns_config.dns_static_zone_file_path = estrdup(Z_STRVAL_P(value));
            } else {
                zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0,
                    "dns_static_zone_file_path must be a string.");
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "dns_recursive_forwarders")) {
            if (Z_TYPE_P(value) == IS_STRING) {
                quicpro_smart_dns_config.dns_recursive_forwarders = estrdup(Z_STRVAL_P(value));
            } else {
                zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0,
                    "dns_recursive_forwarders must be a string.");
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "dns_health_agent_mcp_endpoint")) {
            if (Z_TYPE_P(value) == IS_STRING) {
                quicpro_smart_dns_config.dns_health_agent_mcp_endpoint = estrdup(Z_STRVAL_P(value));
            } else {
                zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0,
                    "dns_health_agent_mcp_endpoint must be a string.");
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "dns_mothernode_uri")) {
            if (Z_TYPE_P(value) == IS_STRING) {
                quicpro_smart_dns_config.dns_mothernode_uri = estrdup(Z_STRVAL_P(value));
            } else {
                zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0,
                    "dns_mothernode_uri must be a string.");
                return FAILURE;
            }
        }
    } ZEND_HASH_FOREACH_END();

    return SUCCESS;
}
