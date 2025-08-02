/*
 * =========================================================================
 * FILENAME:   src/config/smart_contracts/ini.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    "INI settings are the blockchain of PHP startup."
 *
 * PURPOSE:
 * Registers, parses and validates all `php.ini` directives for the
 * Smart‑Contracts configuration module.
 * =========================================================================
 */

#include "include/config/smart_contracts/ini.h"
#include "include/config/smart_contracts/base_layer.h"

#include "php.h"
#include <zend_ini.h>
#include <ext/spl/spl_exceptions.h>

/*
 * Custom OnUpdate handler for positive long values.
 */
static ZEND_INI_MH(OnUpdateSmartContractPositiveLong)
{
    zend_long val = zend_atol(ZSTR_VAL(new_value), ZSTR_LEN(new_value));

    if (val <= 0) {
        zend_throw_exception_ex(
            spl_ce_InvalidArgumentException, 0,
            "Invalid value provided for smart‑contract directive. A positive integer is required.");
        return FAILURE;
    }

    if (zend_string_equals_literal(entry->name, "quicpro.smartcontract_chain_id")) {
        quicpro_smart_contracts_config.chain_id = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.smartcontract_default_gas_limit")) {
        quicpro_smart_contracts_config.default_gas_limit = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.smartcontract_default_gas_price_gwei")) {
        quicpro_smart_contracts_config.default_gas_price_gwei = val;
    }

    return SUCCESS;
}

/*
 * Custom OnUpdate handler for strings that map to allow‑listed DLT providers.
 */
static ZEND_INI_MH(OnUpdateDltProviderString)
{
    const char *allowed[] = {"ethereum", "solana", "fabric", NULL};
    int i;
    for (i = 0; allowed[i]; ++i) {
        if (zend_string_equals_literal(new_value, allowed[i])) {
            quicpro_smart_contracts_config.dlt_provider = estrdup(ZSTR_VAL(new_value));
            return SUCCESS;
        }
    }

    zend_throw_exception_ex(
        spl_ce_InvalidArgumentException, 0,
        "Invalid DLT provider specified for smart‑contract module.");
    return FAILURE;
}

static ZEND_INI_MH(OnUpdateStringDuplicate)
{
    /* Re‑use Zend helper to allocate persistent string copy */
    OnUpdateString(entry, new_value, mh_arg1, mh_arg2, mh_arg3);
    return SUCCESS;
}

PHP_INI_BEGIN()
    /* Master switch */
    STD_PHP_INI_ENTRY("quicpro.smartcontract_enable", "0", PHP_INI_SYSTEM,
        OnUpdateBool, smartcontract_enable, qp_smart_contracts_config_t, quicpro_smart_contracts_config)

    /* Registry & connectivity */
    ZEND_INI_ENTRY_EX("quicpro.smartcontract_registry_uri", "https://contracts.quicpro.internal",
        PHP_INI_SYSTEM, OnUpdateStringDuplicate, &quicpro_smart_contracts_config.registry_uri, NULL, NULL)

    ZEND_INI_ENTRY_EX("quicpro.smartcontract_dlt_provider", "ethereum",
        PHP_INI_SYSTEM, OnUpdateDltProviderString, NULL, NULL, NULL)

    ZEND_INI_ENTRY_EX("quicpro.smartcontract_dlt_rpc_endpoint", "",
        PHP_INI_SYSTEM, OnUpdateStringDuplicate, &quicpro_smart_contracts_config.dlt_rpc_endpoint, NULL, NULL)

    ZEND_INI_ENTRY_EX("quicpro.smartcontract_chain_id", "1",
        PHP_INI_SYSTEM, OnUpdateSmartContractPositiveLong, NULL, NULL, NULL)

    ZEND_INI_ENTRY_EX("quicpro.smartcontract_default_gas_limit", "300000",
        PHP_INI_SYSTEM, OnUpdateSmartContractPositiveLong, NULL, NULL, NULL)

    ZEND_INI_ENTRY_EX("quicpro.smartcontract_default_gas_price_gwei", "20",
        PHP_INI_SYSTEM, OnUpdateSmartContractPositiveLong, NULL, NULL, NULL)

    /* Wallet & key management */
    ZEND_INI_ENTRY_EX("quicpro.smartcontract_default_wallet_path", "",
        PHP_INI_SYSTEM, OnUpdateStringDuplicate, &quicpro_smart_contracts_config.default_wallet_path, NULL, NULL)

    ZEND_INI_ENTRY_EX("quicpro.smartcontract_default_wallet_password_env_var", "QUICPRO_WALLET_PASSWORD",
        PHP_INI_SYSTEM, OnUpdateStringDuplicate, &quicpro_smart_contracts_config.default_wallet_password_env_var, NULL, NULL)

    STD_PHP_INI_ENTRY("quicpro.smartcontract_use_hardware_wallet", "0", PHP_INI_SYSTEM,
        OnUpdateBool, use_hardware_wallet, qp_smart_contracts_config_t, quicpro_smart_contracts_config)

    ZEND_INI_ENTRY_EX("quicpro.smartcontract_hsm_pkcs11_library_path", "/usr/lib/x86_64-linux-gnu/softhsm/libsofthsm2.so",
        PHP_INI_SYSTEM, OnUpdateStringDuplicate, &quicpro_smart_contracts_config.hsm_pkcs11_library_path, NULL, NULL)

    /* ABI directory & event handling */
    ZEND_INI_ENTRY_EX("quicpro.smartcontract_abi_directory", "/var/www/abis",
        PHP_INI_SYSTEM, OnUpdateStringDuplicate, &quicpro_smart_contracts_config.abi_directory, NULL, NULL)

    STD_PHP_INI_ENTRY("quicpro.smartcontract_event_listener_enable", "0", PHP_INI_SYSTEM,
        OnUpdateBool, event_listener_enable, qp_smart_contracts_config_t, quicpro_smart_contracts_config)
PHP_INI_END()

void qp_config_smart_contracts_ini_register(void)
{
    REGISTER_INI_ENTRIES();
}

void qp_config_smart_contracts_ini_unregister(void)
{
    UNREGISTER_INI_ENTRIES();
}
