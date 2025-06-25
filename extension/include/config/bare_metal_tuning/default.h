/*
 * =========================================================================
 * FILENAME:   include/config/bare_metal_tuning/default.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Attack ships on fire off the shoulder of Orion.
 *
 * PURPOSE:
 * This header declares the function for loading the module's hardcoded
 * default values for bare metal tuning.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_BARE_METAL_DEFAULT_H
#define QUICPRO_CONFIG_BARE_METAL_DEFAULT_H

/**
 * @brief Loads the hardcoded, default values into the module's config struct.
 */
void qp_config_bare_metal_tuning_defaults_load(void);

#endif /* QUICPRO_CONFIG_BARE_METAL_DEFAULT_H */
