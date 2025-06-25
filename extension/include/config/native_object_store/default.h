/*
 * =========================================================================
 * FILENAME:   include/config/native_object_store/default.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    One Ring to bring them all...
 *
 * PURPOSE:
 * This header declares the function for loading the module's hardcoded
 * default values for the Native Object Store.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_NATIVE_OBJECT_STORE_DEFAULT_H
#define QUICPRO_CONFIG_NATIVE_OBJECT_STORE_DEFAULT_H

/**
 * @brief Loads the hardcoded, default values into the module's config struct.
 */
void qp_config_native_object_store_defaults_load(void);

#endif /* QUICPRO_CONFIG_NATIVE_OBJECT_STORE_DEFAULT_H */
