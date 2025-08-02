/*
 * =========================================================================
 * FILENAME:   src/config/high_perf_compute_and_ai/config.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    In three years, Cyberdyne will become the largest supplier
 * of military computer systems. All stealth bombers are upgraded
 * with Cyberdyne computers, becoming fully unmanned.
 *
 * PURPOSE:
 * This file implements the logic for applying runtime configuration
 * changes to the High-Performance Compute & AI module.
 * =========================================================================
 */

#include "include/config/high_perf_compute_and_ai/config.h"
#include "include/config/high_perf_compute_and_ai/base_layer.h"
#include "include/quicpro_globals.h"

/* Centralized validation helpers */
#include "include/validation/config_param/validate_bool.h"
#include "include/validation/config_param/validate_positive_long.h"
#include "include/validation/config_param/validate_non_negative_long.h"
#include "include/validation/config_param/validate_string_from_allowlist.h"
#include "include/validation/config_param/validate_cpu_affinity_map_string.h" /* For GPU affinity map */

#include "php.h"
#include <ext/spl/spl_exceptions.h>

int qp_config_high_perf_compute_and_ai_apply_userland_config(zval *config_arr)
{
    zval *value;
    zend_string *key;

    if (!quicpro_globals.is_userland_override_allowed) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Configuration override from userland is disabled by system administrator.");
        return FAILURE;
    }

    if (Z_TYPE_P(config_arr) != IS_ARRAY) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Configuration must be provided as an array.");
        return FAILURE;
    }

    ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(config_arr), key, value) {
        if (!key) continue;

        if (zend_string_equals_literal(key, "dataframe_enable")) {
            if (qp_validate_bool(value) != SUCCESS) return FAILURE;
            quicpro_high_perf_compute_ai_config.dataframe_enable = zend_is_true(value);
        } else if (zend_string_equals_literal(key, "dataframe_memory_limit_mb")) {
            if (qp_validate_positive_long(value, &quicpro_high_perf_compute_ai_config.dataframe_memory_limit_mb) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "dataframe_string_interning_enable")) {
            if (qp_validate_bool(value) != SUCCESS) return FAILURE;
            quicpro_high_perf_compute_ai_config.dataframe_string_interning_enable = zend_is_true(value);
        } else if (zend_string_equals_literal(key, "dataframe_cpu_parallelism_default")) {
            if (qp_validate_non_negative_long(value, &quicpro_high_perf_compute_ai_config.dataframe_cpu_parallelism_default) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "gpu_bindings_enable")) {
            if (qp_validate_bool(value) != SUCCESS) return FAILURE;
            quicpro_high_perf_compute_ai_config.gpu_bindings_enable = zend_is_true(value);
        } else if (zend_string_equals_literal(key, "gpu_default_backend")) {
            const char *allowed[] = {"auto", "cuda", "rocm", "sycl", NULL};
            if (qp_validate_string_from_allowlist(value, allowed, &quicpro_high_perf_compute_ai_config.gpu_default_backend) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "worker_gpu_affinity_map")) {
            /* We can reuse the CPU affinity map validator as the format is identical */
            if (qp_validate_cpu_affinity_map_string(value, &quicpro_high_perf_compute_ai_config.worker_gpu_affinity_map) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "gpu_memory_preallocation_mb")) {
            if (qp_validate_positive_long(value, &quicpro_high_perf_compute_ai_config.gpu_memory_preallocation_mb) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "gpu_p2p_enable")) {
            if (qp_validate_bool(value) != SUCCESS) return FAILURE;
            quicpro_high_perf_compute_ai_config.gpu_p2p_enable = zend_is_true(value);
        } else if (zend_string_equals_literal(key, "storage_enable_directstorage")) {
            if (qp_validate_bool(value) != SUCCESS) return FAILURE;
            quicpro_high_perf_compute_ai_config.storage_enable_directstorage = zend_is_true(value);
        } else if (zend_string_equals_literal(key, "cuda_enable_tensor_cores")) {
            if (qp_validate_bool(value) != SUCCESS) return FAILURE;
            quicpro_high_perf_compute_ai_config.cuda_enable_tensor_cores = zend_is_true(value);
        } else if (zend_string_equals_literal(key, "cuda_stream_pool_size")) {
            if (qp_validate_positive_long(value, &quicpro_high_perf_compute_ai_config.cuda_stream_pool_size) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "rocm_enable_gfx_optimizations")) {
            if (qp_validate_bool(value) != SUCCESS) return FAILURE;
            quicpro_high_perf_compute_ai_config.rocm_enable_gfx_optimizations = zend_is_true(value);
        } else if (zend_string_equals_literal(key, "arc_enable_xmx_optimizations")) {
            if (qp_validate_bool(value) != SUCCESS) return FAILURE;
            quicpro_high_perf_compute_ai_config.arc_enable_xmx_optimizations = zend_is_true(value);
        } else if (zend_string_equals_literal(key, "arc_video_acceleration_enable")) {
            if (qp_validate_bool(value) != SUCCESS) return FAILURE;
            quicpro_high_perf_compute_ai_config.arc_video_acceleration_enable = zend_is_true(value);
        }

    } ZEND_HASH_FOREACH_END();

    return SUCCESS;
}
