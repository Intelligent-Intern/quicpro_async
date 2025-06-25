/*
 * =========================================================================
 * FILENAME:   include/config/dynamic_admin_api/index.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    I'm gonna make him an offer he can't refuse.
 *
 * PURPOSE:
 * This header file declares the public C-API for the dynamic admin API
 * configuration module.
 *
 * ARCHITECTURE:
 * It provides the function prototypes for the module's lifecycle, which
 * are called by the master dispatcher to orchestrate loading of settings.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_DYNAMIC_ADMIN_API_INDEX_H
#define QUICPRO_CONFIG_DYNAMIC_ADMIN_API_INDEX_H

/**
 * @brief Initializes the Dynamic Admin API configuration module.
 */
void qp_config_dynamic_admin_api_init(void);

/**
 * @brief Shuts down the Dynamic Admin API configuration module.
 */
void qp_config_dynamic_admin_api_shutdown(void);

#endif /* QUICPRO_CONFIG_DYNAMIC_ADMIN_API_INDEX_H */
