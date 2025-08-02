/*
 * =========================================================================
 * FILENAME:   src/config/cloud_autoscale/index.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    A process cannot be understood by stopping it.
 *
 * PURPOSE:
 * This file implements the lifecycle functions for the cloud autoscaling
 * configuration module.
 * =========================================================================
 */

#include "include/config/cloud_autoscale/index.h"
#include "include/config/cloud_autoscale/default.h"
#include "include/config/cloud_autoscale/ini.h"
#include "php.h"

/**
 * @brief Initializes the Cloud Autoscaling configuration module.
 */
void qp_config_cloud_autoscale_init(void)
{
    /* Step 1: Load hardcoded, safe defaults. */
    qp_config_cloud_autoscale_defaults_load();

    /* Step 2: Register INI handlers to override defaults. */
    qp_config_cloud_autoscale_ini_register();
}

/**
 * @brief Shuts down the Cloud Autoscaling configuration module.
 */
void qp_config_cloud_autoscale_shutdown(void)
{
    qp_config_cloud_autoscale_ini_unregister();
}
