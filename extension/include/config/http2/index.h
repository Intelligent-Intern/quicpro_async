/*
 * =========================================================================
 * FILENAME:   include/config/http2/index.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Wait a minute, Doc. Are you telling me you built a time machine...
 * out of a DeLorean?
 *
 * PURPOSE:
 * This header file declares the public C-API for the HTTP/2
 * configuration module.
 *
 * ARCHITECTURE:
 * It provides the function prototypes for the module's lifecycle, which
 * are called by the master dispatcher to orchestrate loading of settings.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_HTTP2_INDEX_H
#define QUICPRO_CONFIG_HTTP2_INDEX_H

/**
 * @brief Initializes the HTTP/2 configuration module.
 */
void qp_config_http2_init(void);

/**
 * @brief Shuts down the HTTP/2 configuration module.
 */
void qp_config_http2_shutdown(void);

#endif /* QUICPRO_CONFIG_HTTP2_INDEX_H */
