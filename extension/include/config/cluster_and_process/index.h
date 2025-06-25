/*
 * =========================================================================
 * FILENAME:   include/config/cluster_and_process/index.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Open the pod bay doors, HAL.
 *
 * PURPOSE:
 * This header file declares the public C-API for the `cluster_and_process`
 * configuration module.
 *
 * ARCHITECTURE:
 * It provides the function prototypes for the module's lifecycle, which
 * are called by the master dispatcher (`quicpro_init.c`) during the
 * `MINIT` and `MSHUTDOWN` phases. These functions are responsible for
 * orchestrating the loading of all cluster and process-related settings.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_CLUSTER_INDEX_H
#define QUICPRO_CONFIG_CLUSTER_INDEX_H

/**
 * @brief Initializes the Cluster & Process configuration module.
 * @details This function orchestrates the loading sequence for all cluster
 * and process management settings. It loads defaults, then registers
 * the INI handlers which allow the Zend Engine to override them with
 * values from `php.ini`.
 *
 * This function is called from `quicpro_init_modules()` at startup.
 */
void qp_config_cluster_and_process_init(void);

/**
 * @brief Shuts down the Cluster & Process configuration module.
 * @details This function unregisters the module's `php.ini` settings
 * to ensure a clean shutdown. It is called from
 * `quicpro_shutdown_modules()` during the MSHUTDOWN phase.
 */
void qp_config_cluster_and_process_shutdown(void);

#endif /* QUICPRO_CONFIG_CLUSTER_INDEX_H */
