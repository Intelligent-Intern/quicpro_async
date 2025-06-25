/*
 * =========================================================================
 * FILENAME:   include/config/app_http3_websockets_webtransport/index.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    What if a cyber brain could possibly generate its own ghost,
 * create a soul all by itself?
 *
 * PURPOSE:
 * This header file declares the public C-API for the application protocols
 * configuration module.
 *
 * ARCHITECTURE:
 * It provides the function prototypes for the module's lifecycle, which
 * are called by the master dispatcher to orchestrate loading of settings.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_APP_PROTOCOLS_INDEX_H
#define QUICPRO_CONFIG_APP_PROTOCOLS_INDEX_H

/**
 * @brief Initializes the Application Protocols configuration module.
 */
void qp_config_app_http3_websockets_webtransport_init(void);

/**
 * @brief Shuts down the Application Protocols configuration module.
 */
void qp_config_app_http3_websockets_webtransport_shutdown(void);

#endif /* QUICPRO_CONFIG_APP_PROTOCOLS_INDEX_H */
