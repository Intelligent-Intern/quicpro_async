/*
 * =========================================================================
 * FILENAME:   include/config/iibin/default.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    It is a mistake to think you can solve any major problems
 * just with potatoes.
 *
 * PURPOSE:
 * This header declares the function for loading the module's hardcoded
 * default values for the IIBIN serialization engine.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_IIBIN_DEFAULT_H
#define QUICPRO_CONFIG_IIBIN_DEFAULT_H

/**
 * @brief Loads the hardcoded, default values into the module's config struct.
 */
void qp_config_iibin_defaults_load(void);

#endif /* QUICPRO_CONFIG_IIBIN_DEFAULT_H */
