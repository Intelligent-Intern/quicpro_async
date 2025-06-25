/*
 * =========================================================================
 * FILENAME:   include/config/smart_contracts/base_layer.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    The precogs are never wrong.
 *
 * PURPOSE:
 * This header file defines the core data structure for the
 * `smart_contracts` configuration module.
 *
 * ARCHITECTURE:
 * This struct holds the configuration values for the native C-level smart
 * contract integration, enabling interaction with blockchain networks.
 * =========================================================================
 */
#ifndef QUICPRO_CONFIG_SMART_CONTRACTS_BASE_H
#define QUICPRO_CONFIG_SMART_CONTRACTS_BASE_H

#include "php.h"
#include <stdbool.h>

typedef struct _qp_smart_contracts_config_t {
    /* --- General & Registry --- */
    bool enable;
    char *registry_uri;

    /* --- Blockchain/DLT Connectivity --- */
    char *dlt_provider;
    char *dlt_rpc_endpoint;
    zend_long chain_id;
    zend_long default_gas_limit;
    zend_long default_gas_price_gwei;

    /* --- Wallet & Key Management --- */
    char *default_wallet_path;
    char *default_wallet_password_env_var;
    bool use_hardware_wallet;
    char *hsm_pkcs11_library_path;

    /* --- Application & Event Handling --- */
    char *abi_directory;
    bool event_listener_enable;

} qp_smart_contracts_config_t;

/* The single instance of this module's configuration data */
extern qp_smart_contracts_config_t quicpro_smart_contracts_config;

#endif /* QUICPRO_CONFIG_SMART_CONTRACTS_BASE_H */
