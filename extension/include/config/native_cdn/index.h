/*
 * =========================================================================
 * FILENAME:   include/config/native_cdn/index.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    I fight for the users.
 *
 * PURPOSE:
 * This header file declares the public C-API for the Native CDN
 * configuration module.
 *
 * ARCHITECTURE:
 * It provides the function prototypes for the module's lifecycle, which
 * are called by the master dispatcher to orchestrate loading of settings.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_NATIVE_CDN_INDEX_H
#define QUICPRO_CONFIG_NATIVE_CDN_INDEX_H

/**
 * @brief Initializes the Native CDN configuration module.
 */
void qp_config_native_cdn_init(void);

/**
 * @brief Shuts down the Native CDN configuration module.
 */
void qp_config_native_cdn_shutdown(void);

#endif /* QUICPRO_CONFIG_NATIVE_CDN_INDEX_H */
