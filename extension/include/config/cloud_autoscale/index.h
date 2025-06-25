/*
 * =========================================================================
 * FILENAME:   include/config/cloud_autoscale/index.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    The spice must flow.
 *
 * PURPOSE:
 * This header file declares the public C-API for the cloud autoscaling
 * configuration module.
 *
 * ARCHITECTURE:
 * It provides the function prototypes for the module's lifecycle, which
 * are called by the master dispatcher to orchestrate loading of settings.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_CLOUD_AUTOSCALE_INDEX_H
#define QUICPRO_CONFIG_CLOUD_AUTOSCALE_INDEX_H

/**
 * @brief Initializes the Cloud Autoscaling configuration module.
 */
void qp_config_cloud_autoscale_init(void);

/**
 * @brief Shuts down the Cloud Autoscaling configuration module.
 */
void qp_config_cloud_autoscale_shutdown(void);

#endif /* QUICPRO_CONFIG_CLOUD_AUTOSCALE_INDEX_H */
