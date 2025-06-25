/*
 * =========================================================================
 * FILENAME:   include/config/cloud_autoscale/default.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    A beginning is a very delicate time.
 *
 * PURPOSE:
 * This header declares the function for loading the module's hardcoded
 * default values for cloud autoscaling.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_CLOUD_AUTOSCALE_DEFAULT_H
#define QUICPRO_CONFIG_CLOUD_AUTOSCALE_DEFAULT_H

/**
 * @brief Loads the hardcoded, default values into the module's config struct.
 */
void qp_config_cloud_autoscale_defaults_load(void);

#endif /* QUICPRO_CONFIG_CLOUD_AUTOSCALE_DEFAULT_H */
