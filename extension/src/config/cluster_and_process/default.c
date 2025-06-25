/*
 * =========================================================================
 * FILENAME:   src/config/cluster_and_process/default.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    I think you know what the problem is just as well as I do.
 *
 * PURPOSE:
 * This file implements the function that loads the hardcoded, default
 * values into the `cluster_and_process` configuration struct.
 *
 * ARCHITECTURE:
 * This is the first and most fundamental step in this module's configuration
 * loading sequence. It establishes a known, safe baseline before any
 * external configuration (like php.ini) is considered. The defaults
 * are chosen to be both functional and conservative.
 * =========================================================================
 */

#include "include/config/cluster_and_process/default.h"
#include "include/config/cluster_and_process/base_layer.h"
#include "php.h"

/**
 * @brief Loads the hardcoded, default values into the module's
 * config struct. This is the first step in the configuration load sequence.
 */
void qp_config_cluster_and_process_defaults_load(void)
{
    /* --- Core Cluster Settings --- */
    /* Default to 0, which enables auto-detection of CPU cores. */
    quicpro_cluster_config.cluster_workers = 0;
    /* A generous 30-second grace period for clean shutdown. */
    quicpro_cluster_config.cluster_graceful_shutdown_ms = 30000;
    /* A reasonable default for open file descriptors. */
    quicpro_cluster_config.server_max_fd_per_worker = 8192;

    /* --- Cluster Resilience & Worker Performance Tuning (Advanced) --- */
    /* Enable self-healing by default. */
    quicpro_cluster_config.cluster_restart_crashed_workers = true;
    /* Sensible defaults to prevent crash loops. */
    quicpro_cluster_config.cluster_max_restarts_per_worker = 10;
    quicpro_cluster_config.cluster_restart_interval_sec = 60;
    /* Default to standard process priority. */
    quicpro_cluster_config.cluster_worker_niceness = 0;
    /* Default to the standard Linux scheduler. */
    quicpro_cluster_config.cluster_worker_scheduler_policy = pestrdup("other", 1);

    /*
     * Default advanced affinity/security settings to empty strings,
     * indicating they are not set. The functional modules must check
     * for non-empty strings before attempting to apply these settings.
     */
    quicpro_cluster_config.cluster_worker_cpu_affinity_map = pestrdup("", 1);
    quicpro_cluster_config.cluster_worker_cgroup_path = pestrdup("", 1);
    quicpro_cluster_config.cluster_worker_user_id = pestrdup("", 1);
    quicpro_cluster_config.cluster_worker_group_id = pestrdup("", 1);
}
