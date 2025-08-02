/*
 * =========================================================================
 * FILENAME:   src/config/smart_contracts/config.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    "Gas limits are low‑level, but validation is higher order."
 *
 * PURPOSE:
 * Implements runtime configuration changes for Smart‑Contracts module.
 * =========================================================================
 */

#include "include/config/smart_contracts/config.h"
#include "include/config/smart_contracts/base_layer.h"
#include "include/quicpro_globals.h"

/* Centralized validation helpers */
#include "include/validation/config_param/validate_bool.h"
#include "include/validation/config_param/validate_positive_long.h"
#include "include/validation/config_param/validate_string_from_allowlist.h"
#include "include/validation/config_param/validate_non_empty_string.h"

#include "php.h"
#include <ext/spl/spl_exceptions.h>

int qp_config_smart_contracts_apply_userland_config(zval *config_arr)
{
    zval *value;
    zend_string *key;

    /* 1. Enforce global security policy. */
    if (!quicpro_globals.is_userland_override_allowed) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0,
            "Configuration override from userland is disabled by system administrator.");
        return FAILURE;
    }

    /* 2. Ensure configuration is an array. */
    if (Z_TYPE_P(config_arr) != IS_ARRAY) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0,
            "Configuration must be provided as an array.");
        return FAILURE;
    }

    /* 3. Iterate, validate and apply each setting. */
    ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(config_arr), key, value) {
        if (!key) {
            continue;
        }

        if (zend_string_equals_literal(key, "smartcontract_enable")) {
            if (qp_validate_bool(value, "smartcontract_enable") == SUCCESS) {
                quicpro_smart_contracts_config.smartcontract_enable = zend_is_true(value);
            } else { return FAILURE; }

        } else if (zend_string_equals_literal(key, "smartcontract_registry_uri")) {
            if (qp_validate_non_empty_string(value, &quicpro_smart_contracts_config.registry_uri) != SUCCESS) {
                return FAILURE;
            }

        } else if (zend_string_equals_literal(key, "smartcontract_dlt_provider")) {
            const char *allowed[] = {"ethereum", "solana", "fabric", NULL};
            if (qp_validate_string_from_allowlist(value, allowed,
                    &quicpro_smart_contracts_config.dlt_provider) != SUCCESS) {
                return FAILURE;
            }

        } else if (zend_string_equals_literal(key, "smartcontract_dlt_rpc_endpoint")) {
            if (qp_validate_non_empty_string(value, &quicpro_smart_contracts_config.dlt_rpc_endpoint) != SUCCESS) {
                return FAILURE;
            }

        } else if (zend_string_equals_literal(key, "smartcontract_chain_id")) {
            if (qp_validate_positive_long(value, &quicpro_smart_contracts_config.chain_id) != SUCCESS) {
                return FAILURE;
            }

        } else if (zend_string_equals_literal(key, "smartcontract_default_gas_limit")) {
            if (qp_validate_positive_long(value, &quicpro_smart_contracts_config.default_gas_limit) != SUCCESS) {
                return FAILURE;
            }

        } else if (zend_string_equals_literal(key, "smartcontract_default_gas_price_gwei")) {
            if (qp_validate_positive_long(value, &quicpro_smart_contracts_config.default_gas_price_gwei) != SUCCESS) {
                return FAILURE;
            }

        } else if (zend_string_equals_literal(key, "smartcontract_default_wallet_path")) {
            if (qp_validate_non_empty_string(value, &quicpro_smart_contracts_config.default_wallet_path) != SUCCESS) {
                return FAILURE;
            }

        } else if (zend_string_equals_literal(key, "smartcontract_default_wallet_password_env_var")) {
            if (qp_validate_non_empty_string(value, &quicpro_smart_contracts_config.default_wallet_password_env_var) != SUCCESS) {
                return FAILURE;
            }

        } else if (zend_string_equals_literal(key, "smartcontract_use_hardware_wallet")) {
            if (qp_validate_bool(value, "smartcontract_use_hardware_wallet") == SUCCESS) {
                quicpro_smart_contracts_config.use_hardware_wallet = zend_is_true(value);
            } else { return FAILURE; }

        } else if (zend_string_equals_literal(key, "smartcontract_hsm_pkcs11_library_path")) {
            if (qp_validate_non_empty_string(value, &quicpro_smart_contracts_config.hsm_pkcs11_library_path) != SUCCESS) {
                return FAILURE;
            }

        } else if (zend_string_equals_literal(key, "smartcontract_abi_directory")) {
            if (qp_validate_non_empty_string(value, &quicpro_smart_contracts_config.abi_directory) != SUCCESS) {
                return FAILURE;
            }

        } else if (zend_string_equals_literal(key, "smartcontract_event_listener_enable")) {
            if (qp_validate_bool(value, "smartcontract_event_listener_enable") == SUCCESS) {
                quicpro_smart_contracts_config.event_listener_enable = zend_is_true(value);
            } else { return FAILURE; }
        }

    } ZEND_HASH_FOREACH_END();

    return SUCCESS;
}
