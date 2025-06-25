/*
 * =========================================================================
 * FILENAME:   src/config/cluster_and_process/ini.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    This mission is too important for me to allow you to jeopardize it.
 *
 * PURPOSE:
 * This file implements the registration, parsing, and validation of all
 * `php.ini` settings for the `cluster_and_process` module.
 *
 * ARCHITECTURE:
 * This file defines custom `OnUpdate...` handlers to ensure that all
 * numeric values from `php.ini` are within a valid, non-negative range.
 * The `PHP_INI_BEGIN` block then maps each directive to either a standard
 * handler (for booleans and basic strings) or one of our custom, more
 * secure validation handlers.
 * =========================================================================
 */

#include "include/config/cluster_and_process/ini.h"
#include "include/config/cluster_and_process/base_layer.h"

#include "php.h"
#include <ext/spl/spl_exceptions.h> /* For spl_ce_InvalidArgumentException */
#include <zend_ini.h>

/*
 * Custom OnUpdate handler for numeric values that must not be negative.
 */
static ZEND_INI_MH(OnUpdateClusterNonNegativeLong)
{
    zend_long val = zend_atol(ZSTR_VAL(new_value), ZSTR_LEN(new_value));

    if (val < 0) {
        zend_throw_exception_ex(
            spl_ce_InvalidArgumentException,
            0,
            "Invalid value provided for a cluster directive. A non-negative integer is required."
        );
        return FAILURE;
    }

    if (zend_string_equals_literal(entry->name, "quicpro.cluster_workers")) {
        quicpro_cluster_config.cluster_workers = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.cluster_graceful_shutdown_ms")) {
        quicpro_cluster_config.cluster_graceful_shutdown_ms = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.server_max_fd_per_worker")) {
        quicpro_cluster_config.server_max_fd_per_worker = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.cluster_max_restarts_per_worker")) {
        quicpro_cluster_config.cluster_max_restarts_per_worker = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.cluster_restart_interval_sec")) {
        quicpro_cluster_config.cluster_restart_interval_sec = val;
    }

    return SUCCESS;
}

/*
 * Here we define all php.ini entries this module is responsible for.
 */
PHP_INI_BEGIN()
    /* --- Core Cluster Settings --- */
    ZEND_INI_ENTRY_EX("quicpro.cluster_workers", "0", PHP_INI_SYSTEM, OnUpdateClusterNonNegativeLong, NULL, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.cluster_graceful_shutdown_ms", "30000", PHP_INI_SYSTEM, OnUpdateClusterNonNegativeLong, NULL, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.server_max_fd_per_worker", "8192", PHP_INI_SYSTEM, OnUpdateClusterNonNegativeLong, NULL, NULL, NULL)

    /* --- Cluster Resilience & Worker Performance Tuning (Advanced) --- */
    STD_PHP_INI_ENTRY("quicpro.cluster_restart_crashed_workers", "1", PHP_INI_SYSTEM, OnUpdateBool, cluster_restart_crashed_workers, qp_cluster_config_t, quicpro_cluster_config)
    ZEND_INI_ENTRY_EX("quicpro.cluster_max_restarts_per_worker", "10", PHP_INI_SYSTEM, OnUpdateClusterNonNegativeLong, NULL, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.cluster_restart_interval_sec", "60", PHP_INI_SYSTEM, OnUpdateClusterNonNegativeLong, NULL, NULL, NULL)

    STD_PHP_INI_ENTRY("quicpro.cluster_worker_niceness", "0", PHP_INI_SYSTEM, OnUpdateLong, cluster_worker_niceness, qp_cluster_config_t, quicpro_cluster_config)
    STD_PHP_INI_ENTRY("quicpro.cluster_worker_scheduler_policy", "other", PHP_INI_SYSTEM, OnUpdateString, cluster_worker_scheduler_policy, qp_cluster_config_t, quicpro_cluster_config)
    STD_PHP_INI_ENTRY("quicpro.cluster_worker_cpu_affinity_map", "", PHP_INI_SYSTEM, OnUpdateString, cluster_worker_cpu_affinity_map, qp_cluster_config_t, quicpro_cluster_config)
    STD_PHP_INI_ENTRY("quicpro.cluster_worker_cgroup_path", "", PHP_INI_SYSTEM, OnUpdateString, cluster_worker_cgroup_path, qp_cluster_config_t, quicpro_cluster_config)
    STD_PHP_INI_ENTRY("quicpro.cluster_worker_user_id", "", PHP_INI_SYSTEM, OnUpdateString, cluster_worker_user_id, qp_cluster_config_t, quicpro_cluster_config)
    STD_PHP_INI_ENTRY("quicpro.cluster_worker_group_id", "", PHP_INI_SYSTEM, OnUpdateString, cluster_worker_group_id, qp_cluster_config_t, quicpro_cluster_config)
PHP_INI_END()

/**
 * @brief Registers this module's php.ini entries with the Zend Engine.
 */
void qp_config_cluster_and_process_ini_register(void)
{
    REGISTER_INI_ENTRIES();
}

/**
 * @brief Unregisters this module's php.ini entries from the Zend Engine.
 */
void qp_config_cluster_and_process_ini_unregister(void)
{
    UNREGISTER_INI_ENTRIES();
}
