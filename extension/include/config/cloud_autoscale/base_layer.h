/*
 * =========================================================================
 * FILENAME:   include/config/cloud_autoscale/base_layer.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    He who can destroy a thing, controls a thing.
 *
 * PURPOSE:
 * This header file defines the core data structure for the
 * `cloud_autoscale` configuration module.
 *
 * ARCHITECTURE:
 * This struct holds the configuration values for the native cloud
 * autoscaling capabilities of the Quicpro\Cluster supervisor after
 * they have been loaded.
 * =========================================================================
 */
#ifndef QUICPRO_CONFIG_CLOUD_AUTOSCALE_BASE_H
#define QUICPRO_CONFIG_CLOUD_AUTOSCALE_BASE_H

#include "php.h"
#include <stdbool.h>

typedef struct _qp_cloud_autoscale_config_t {
    /* --- Provider & Credentials --- */
    char *provider;
    char *region;
    char *credentials_path;

    /* --- Scaling Triggers & Policy --- */
    zend_long min_nodes;
    zend_long max_nodes;
    zend_long scale_up_cpu_threshold_percent;
    zend_long scale_down_cpu_threshold_percent;
    char *scale_up_policy;
    zend_long cooldown_period_sec;
    zend_long idle_node_timeout_sec;

    /* --- Node Provisioning --- */
    char *instance_type;
    char *instance_image_id;
    char *network_config;
    char *instance_tags;

} qp_cloud_autoscale_config_t;

/* The single instance of this module's configuration data */
extern qp_cloud_autoscale_config_t quicpro_cloud_autoscale_config;

#endif /* QUICPRO_CONFIG_CLOUD_AUTOSCALE_BASE_H */
