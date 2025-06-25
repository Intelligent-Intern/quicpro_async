/*
 * =========================================================================
 * FILENAME:   src/config/cluster_and_process/index.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    I'm sorry, Dave. I'm afraid I can't do that.
 *
 * PURPOSE:
 * This file implements the lifecycle functions for the `cluster_and_process`
 * configuration module, as declared in its corresponding `index.h` file.
 *
 * ARCHITECTURE:
 * This is the conductor for this module. It dictates the precise order of
 * operations for loading the configuration:
 *
 * 1. `qp_config_cluster_and_process_defaults_load()`:
 * Loads hardcoded, safe default values into the local config struct.
 *
 * 2. `qp_config_cluster_and_process_ini_register()`:
 * Registers the INI handlers, which allows the Zend Engine to override
 * the defaults with values from `php.ini`.
 * =========================================================================
 */

#include "php.h"

#include "include/config/cluster_and_process/index.h"
#include "include/config/cluster_and_process/default.h"
#include "include/config/cluster_and_process/ini.h"

/**
 * @brief Initializes the Cluster & Process configuration module.
 * @details This function orchestrates the loading sequence for all cluster
 * and process management settings. It loads defaults, then registers
 * the INI handlers which allow the Zend Engine to override them with
 * values from `php.ini`.
 */
void qp_config_cluster_and_process_init(void)
{
    /* Step 1: Populate the config struct with hardcoded, safe defaults. */
    qp_config_cluster_and_process_defaults_load();

    /*
     * Step 2: Register the php.ini handlers. The Zend Engine will now
     * automatically parse the ini file, calling our OnUpdate handlers
     * and overriding the default values.
     */
    qp_config_cluster_and_process_ini_register();
}

/**
 * @brief Shuts down the Cluster & Process configuration module.
 * @details This function calls the `qp_config_cluster_and_process_ini_unregister`
 * function to instruct the Zend Engine to remove this module's `php.ini`
 * entries during the shutdown sequence.
 */
void qp_config_cluster_and_process_shutdown(void)
{
    qp_config_cluster_and_process_ini_unregister();
}
