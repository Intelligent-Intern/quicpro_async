/*
 * =========================================================================
 * FILENAME:   src/config/cloud_autoscale/default.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    The sleeper must awaken.
 *
 * PURPOSE:
 * This file implements the function that loads the hardcoded, default
 * values into the cloud autoscaling configuration struct.
 * =========================================================================
 */

#include "include/config/cloud_autoscale/default.h"
#include "include/config/cloud_autoscale/base_layer.h"
#include "php.h"

/**
 * @brief Loads the hardcoded, default values into the module's config struct.
 */
void qp_config_cloud_autoscale_defaults_load(void)
{
    /* --- Provider & Credentials --- */
    /* Default to disabled. User must explicitly configure a provider. */
    quicpro_cloud_autoscale_config.provider = pestrdup("", 1);
    quicpro_cloud_autoscale_config.region = pestrdup("", 1);
    quicpro_cloud_autoscale_config.credentials_path = pestrdup("", 1);

    /* --- Scaling Triggers & Policy --- */
    quicpro_cloud_autoscale_config.min_nodes = 1;
    quicpro_cloud_autoscale_config.max_nodes = 1; /* Default to no scaling. */
    quicpro_cloud_autoscale_config.scale_up_cpu_threshold_percent = 80;
    quicpro_cloud_autoscale_config.scale_down_cpu_threshold_percent = 20;
    quicpro_cloud_autoscale_config.scale_up_policy = pestrdup("add_nodes:1", 1);
    quicpro_cloud_autoscale_config.cooldown_period_sec = 300;
    quicpro_cloud_autoscale_config.idle_node_timeout_sec = 600;

    /* --- Node Provisioning --- */
    quicpro_cloud_autoscale_config.instance_type = pestrdup("", 1);
    quicpro_cloud_autoscale_config.instance_image_id = pestrdup("", 1);
    quicpro_cloud_autoscale_config.network_config = pestrdup("", 1);
    quicpro_cloud_autoscale_config.instance_tags = pestrdup("", 1);
}
