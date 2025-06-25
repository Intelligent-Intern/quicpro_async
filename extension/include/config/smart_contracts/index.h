/*
 * =========================================================================
 * FILENAME:   include/config/smart_contracts/index.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Everybody runs.
 *
 * PURPOSE:
 * This header file declares the public C-API for the Smart Contracts
 * configuration module.
 *
 * ARCHITECTURE:
 * It provides the function prototypes for the module's lifecycle, which
 * are called by the master dispatcher to orchestrate loading of settings.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_SMART_CONTRACTS_INDEX_H
#define QUICPRO_CONFIG_SMART_CONTRACTS_INDEX_H

/**
 * @brief Initializes the Smart Contracts configuration module.
 */
void qp_config_smart_contracts_init(void);

/**
 * @brief Shuts down the Smart Contracts configuration module.
 */
void qp_config_smart_contracts_shutdown(void);

#endif /* QUICPRO_CONFIG_SMART_CONTRACTS_INDEX_H */
