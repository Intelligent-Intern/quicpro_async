/*
 * =========================================================================
 * FILENAME:   include/config/state_management/default.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Downwards is the only way forwards.
 *
 * PURPOSE:
 * This header declares the function for loading the module's hardcoded
 * default values for the State Management backend.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_STATE_MANAGEMENT_DEFAULT_H
#define QUICPRO_CONFIG_STATE_MANAGEMENT_DEFAULT_H

/**
 * @brief Loads the hardcoded, default values into the module's config struct.
 */
void qp_config_state_management_defaults_load(void);

#endif /* QUICPRO_CONFIG_STATE_MANAGEMENT_DEFAULT_H */
