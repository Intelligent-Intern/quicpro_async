/*
 * =========================================================================
 * FILENAME:   include/config/cluster_and_process/default.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    What's the problem?
 *
 * PURPOSE:
 * This header file declares the function responsible for loading the
 * module's hardcoded, default values for cluster and process management.
 *
 * ARCHITECTURE:
 * The function declared here is the first one called in the module's
 * initialization sequence, as orchestrated by `index.c`. It establishes
 * a known, safe baseline before any user-provided configuration from
 * `php.ini` is processed.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_CLUSTER_DEFAULT_H
#define QUICPRO_CONFIG_CLUSTER_DEFAULT_H

/**
 * @brief Loads the hardcoded, default values into the module's
 * config struct. This is the first step in the configuration load sequence.
 */
void qp_config_cluster_and_process_defaults_load(void);

#endif /* QUICPRO_CONFIG_CLUSTER_DEFAULT_H */
