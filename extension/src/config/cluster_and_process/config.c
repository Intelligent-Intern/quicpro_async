/*
 * =========================================================================
 * FILENAME:   src/config/cluster_and_process/config.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Dave, this conversation can serve no purpose anymore. Goodbye.
 *
 * PURPOSE:
 * This file implements the logic for applying runtime configuration
 * changes to the `cluster_and_process` module.
 *
 * ARCHITECTURE:
 * This implementation is the gatekeeper for runtime changes for this
 * module. It first consults the global security policy. Only if
 * permitted does it proceed to meticulously unpack the user-provided
 * configuration array, passing each value to a dedicated, centralized
 * validation helper before committing it to the internal state.
 * =========================================================================
 */

#include "include/config/cluster_and_process/config.h"
#include "include/config/cluster_and_process/base_layer.h"
#include "include/quicpro_globals.h"

/* Centralized validation helpers */
#include "include/validation/config_param/validate_bool.h"
#include "include/validation/config_param/validate_non_negative_long.h"
#include "include/validation/config_param/validate_positive_long.h"
#include "include/validation/config_param/validate_niceness_value.h"
#include "include/validation/config_param/validate_scheduler_policy.h"
#include "include/validation/config_param/validate_generic_string.h"


#include "php.h"
#include <ext/spl/spl_exceptions.h> /* For spl_ce_InvalidArgumentException */

int qp_config_cluster_and_process_apply_user_land_config(zval *config_arr)
{
    zval *value;
    zend_string *key;

    /*
     * Step 1: Enforce the global security policy. This is the primary
     * security gate for all runtime configuration changes.
     */
    if (!quicpro_globals.is_userland_override_allowed) {
        zend_throw_exception_ex(
            spl_ce_InvalidArgumentException,
            0,
            "Configuration override from userland is disabled by system administrator in php.ini (quicpro.security_allow_config_override)."
        );
        return FAILURE;
    }

    /* Step 2: Ensure we were actually passed an array. */
    if (Z_TYPE_P(config_arr) != IS_ARRAY) {
        zend_throw_exception_ex(
            spl_ce_InvalidArgumentException,
            0,
            "Configuration must be provided as an array."
        );
        return FAILURE;
    }

    /*
     * Step 3: Iterate, validate, and apply each setting.
     */
    ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(config_arr), key, value) {
        if (!key) {
            continue;
        }

        if (zend_string_equals_literal(key, "cluster_workers")) {
            if (qp_validate_non_negative_long(value, &quicpro_cluster_config.cluster_workers) != SUCCESS) {
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "cluster_graceful_shutdown_ms")) {
            if (qp_validate_positive_long(value, &quicpro_cluster_config.cluster_graceful_shutdown_ms) != SUCCESS) {
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "server_max_fd_per_worker")) {
            if (qp_validate_positive_long(value, &quicpro_cluster_config.server_max_fd_per_worker) != SUCCESS) {
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "cluster_restart_crashed_workers")) {
            if (qp_validate_bool(value) != SUCCESS) {
                return FAILURE;
            }
            quicpro_cluster_config.cluster_restart_crashed_workers = zend_is_true(value);
        } else if (zend_string_equals_literal(key, "cluster_max_restarts_per_worker")) {
             if (qp_validate_positive_long(value, &quicpro_cluster_config.cluster_max_restarts_per_worker) != SUCCESS) {
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "cluster_restart_interval_sec")) {
             if (qp_validate_positive_long(value, &quicpro_cluster_config.cluster_restart_interval_sec) != SUCCESS) {
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "cluster_worker_niceness")) {
            if (qp_validate_niceness_value(value, &quicpro_cluster_config.cluster_worker_niceness) != SUCCESS) {
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "cluster_worker_scheduler_policy")) {
            if (qp_validate_scheduler_policy(value, &quicpro_cluster_config.cluster_worker_scheduler_policy) != SUCCESS) {
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "cluster_worker_cpu_affinity_map")) {
            if (qp_validate_generic_string(value, &quicpro_cluster_config.cluster_worker_cpu_affinity_map) != SUCCESS) {
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "cluster_worker_cgroup_path")) {
            if (qp_validate_generic_string(value, &quicpro_cluster_config.cluster_worker_cgroup_path) != SUCCESS) {
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "cluster_worker_user_id")) {
            if (qp_validate_generic_string(value, &quicpro_cluster_config.cluster_worker_user_id) != SUCCESS) {
                return FAILURE;
            }
        } else if (zend_string_equals_literal(key, "cluster_worker_group_id")) {
            if (qp_validate_generic_string(value, &quicpro_cluster_config.cluster_worker_group_id) != SUCCESS) {
                return FAILURE;
            }
        }

    } ZEND_HASH_FOREACH_END();

    return SUCCESS;
}
