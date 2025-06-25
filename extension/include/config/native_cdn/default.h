/*
 * =========================================================================
 * FILENAME:   include/config/native_cdn/default.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    On the other side of the screen, it all looks so easy.
 *
 * PURPOSE:
 * This header declares the function for loading the module's hardcoded
 * default values for the Native CDN.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_NATIVE_CDN_DEFAULT_H
#define QUICPRO_CONFIG_NATIVE_CDN_DEFAULT_H

/**
 * @brief Loads the hardcoded, default values into the module's config struct.
 */
void qp_config_native_cdn_defaults_load(void);

#endif /* QUICPRO_CONFIG_NATIVE_CDN_DEFAULT_H */
