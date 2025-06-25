/*
 * =========================================================================
 * FILENAME:   include/config/http2/default.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    1.21 gigawatts! 1.21 gigawatts! Great Scott!
 *
 * PURPOSE:
 * This header declares the function for loading the module's hardcoded
 * default values for the HTTP/2 protocol engine.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_HTTP2_DEFAULT_H
#define QUICPRO_CONFIG_HTTP2_DEFAULT_H

/**
 * @brief Loads the hardcoded, default values into the module's config struct.
 */
void qp_config_http2_defaults_load(void);

#endif /* QUICPRO_CONFIG_HTTP2_DEFAULT_H */
