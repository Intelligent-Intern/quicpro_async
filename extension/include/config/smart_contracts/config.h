/*
 * =========================================================================
 * FILENAME:   include/config/smart_contracts/config.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    The fact that you prevented it from happening doesn't change
 * the fact that it was going to happen.
 *
 * PURPOSE:
 * This header file declares the public C-API for applying runtime
 * configuration changes from PHP userland to the Smart Contracts module.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_SMART_CONTRACTS_CONFIG_H
#define QUICPRO_CONFIG_SMART_CONTRACTS_CONFIG_H

#include "php.h"

/**
 * @brief Applies runtime configuration settings from a PHP array.
 * @param config_arr A zval pointer to a PHP array containing the key-value
 * pairs of the configuration to apply.
 * @return `SUCCESS` if all settings were successfully validated and applied,
 * `FAILURE` otherwise.
 */
int qp_config_smart_contracts_apply_userland_config(zval *config_arr);

#endif /* QUICPRO_CONFIG_SMART_CONTRACTS_CONFIG_H */
