/*
 * =========================================================================
 * FILENAME:   src/config/smart_contracts/default.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    "Even blockchains start with defaults."
 *
 * PURPOSE:
 * Implements the function that loads the hard‑coded, conservative default
 * values for the Smart‑Contracts configuration module.
 * =========================================================================
 */

#include "include/config/smart_contracts/default.h"
#include "include/config/smart_contracts/base_layer.h"
#include "php.h"

void qp_config_smart_contracts_defaults_load(void)
{
    quicpro_smart_contracts_config.smartcontract_enable = false;
    quicpro_smart_contracts_config.registry_uri = pestrdup("https://contracts.quicpro.internal", 1);

    quicpro_smart_contracts_config.dlt_provider = pestrdup("ethereum", 1);
    quicpro_smart_contracts_config.dlt_rpc_endpoint = pestrdup("", 1);
    quicpro_smart_contracts_config.chain_id = 1;
    quicpro_smart_contracts_config.default_gas_limit = 300000;
    quicpro_smart_contracts_config.default_gas_price_gwei = 20;

    quicpro_smart_contracts_config.default_wallet_path = pestrdup("", 1);
    quicpro_smart_contracts_config.default_wallet_password_env_var = pestrdup("QUICPRO_WALLET_PASSWORD", 1);
    quicpro_smart_contracts_config.use_hardware_wallet = false;
    quicpro_smart_contracts_config.hsm_pkcs11_library_path = pestrdup("/usr/lib/x86_64-linux-gnu/softhsm/libsofthsm2.so", 1);

    quicpro_smart_contracts_config.abi_directory = pestrdup("/var/www/abis", 1);
    quicpro_smart_contracts_config.event_listener_enable = false;
}
