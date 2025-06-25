/*
 * =========================================================================
 * FILENAME:   include/config/cluster_and_process/ini.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    What are you talking about, HAL?
 *
 * PURPOSE:
 * This header file declares the internal C-API for handling the `php.ini`
 * entries of the `cluster_and_process` configuration module.
 *
 * ARCHITECTURE:
 * These functions are considered internal to this module. They are called
 * by the module's `index.c` orchestrator and provide the direct interface
 * to the Zend Engine's INI system.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_CLUSTER_INI_H
#define QUICPRO_CONFIG_CLUSTER_INI_H

/**
 * @brief Registers this module's php.ini entries with the Zend Engine.
 * @details This function is called by the module's index during the MINIT
 * phase. It makes all cluster and process-related INI directives known
 * to PHP and triggers the custom OnUpdate handlers that perform validation.
 */
void qp_config_cluster_and_process_ini_register(void);

/**
 * @brief Unregisters this module's php.ini entries from the Zend Engine.
 * @details This function is called by the module's index during the
 * MSHUTDOWN phase to ensure a clean release of resources.
 */
void qp_config_cluster_and_process_ini_unregister(void);

#endif /* QUICPRO_CONFIG_CLUSTER_INI_H */
