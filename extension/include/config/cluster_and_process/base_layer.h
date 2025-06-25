/*
 * =========================================================================
 * FILENAME:   include/config/cluster_and_process/base_layer.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    My mind is going. I can feel it.
 *
 * PURPOSE:
 * This header file defines the core data structure for the
 * `cluster_and_process` configuration module.
 *
 * ARCHITECTURE:
 * This struct holds the actual configuration values for this module after
 * they have been loaded. The `extern` declaration makes the single global
 * instance of this struct available to all files within the module.
 * =========================================================================
 */
#ifndef QUICPRO_CONFIG_CLUSTER_BASE_H
#define QUICPRO_CONFIG_CLUSTER_BASE_H

#include "php.h"
#include <stdbool.h>

typedef struct _qp_cluster_config_t {
    /* --- Core Cluster Settings --- */
    zend_long cluster_workers;
    zend_long cluster_graceful_shutdown_ms;
    zend_long server_max_fd_per_worker;

    /* --- Cluster Resilience & Worker Performance Tuning (Advanced) --- */
    bool cluster_restart_crashed_workers;
    zend_long cluster_max_restarts_per_worker;
    zend_long cluster_restart_interval_sec;
    zend_long cluster_worker_niceness;
    char *cluster_worker_scheduler_policy;
    char *cluster_worker_cpu_affinity_map;
    char *cluster_worker_cgroup_path;
    char *cluster_worker_user_id;
    char *cluster_worker_group_id;

} qp_cluster_config_t;

/* The single instance of this module's configuration data */
extern qp_cluster_config_t quicpro_cluster_config;

#endif /* QUICPRO_CONFIG_CLUSTER_BASE_H */
