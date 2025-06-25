/*
 * =========================================================================
 * FILENAME:   include/config/native_object_store/index.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    One Ring to find them...
 *
 * PURPOSE:
 * This header file declares the public C-API for the Native Object Store
 * configuration module.
 *
 * ARCHITECTURE:
 * It provides the function prototypes for the module's lifecycle, which
 * are called by the master dispatcher to orchestrate loading of settings.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_NATIVE_OBJECT_STORE_INDEX_H
#define QUICPRO_CONFIG_NATIVE_OBJECT_STORE_INDEX_H

/**
 * @brief Initializes the Native Object Store configuration module.
 */
void qp_config_native_object_store_init(void);

/**
 * @brief Shuts down the Native Object Store configuration module.
 */
void qp_config_native_object_store_shutdown(void);

#endif /* QUICPRO_CONFIG_NATIVE_OBJECT_STORE_INDEX_H */
