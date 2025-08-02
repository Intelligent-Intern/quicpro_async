/*
 * =========================================================================
 * FILENAME:   src/config/high_perf_compute_and_ai/ini.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Skynet begins to learn at a geometric rate. It becomes
 * self-aware at 2:14 a.m. Eastern time, August 29th.
 *
 * PURPOSE:
 * This file implements the registration and parsing of all `php.ini`
 * settings for the High-Performance Compute & AI module.
 * =========================================================================
 */

#include "include/config/high_perf_compute_and_ai/ini.h"
#include "include/config/high_perf_compute_and_ai/base_layer.h"

#include "php.h"
#include <zend_ini.h>
#include <ext/spl/spl_exceptions.h>

/* Custom OnUpdate handler for positive integer values */
static ZEND_INI_MH(OnUpdateAiPositiveLong)
{
    zend_long val = zend_atol(ZSTR_VAL(new_value), ZSTR_LEN(new_value));
    if (val <= 0) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Invalid value for an AI/Compute directive. A positive integer is required.");
        return FAILURE;
    }

    if (zend_string_equals_literal(entry->name, "quicpro.dataframe_memory_limit_mb")) {
        quicpro_high_perf_compute_ai_config.dataframe_memory_limit_mb = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.gpu_memory_preallocation_mb")) {
        quicpro_high_perf_compute_ai_config.gpu_memory_preallocation_mb = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.cuda_stream_pool_size")) {
        quicpro_high_perf_compute_ai_config.cuda_stream_pool_size = val;
    }
    return SUCCESS;
}

/* Custom OnUpdate handler for GPU backend string */
static ZEND_INI_MH(OnUpdateGpuBackend)
{
    const char *allowed[] = {"auto", "cuda", "rocm", "sycl", NULL};
    bool is_allowed = false;
    for (int i = 0; allowed[i] != NULL; i++) {
        if (strcasecmp(ZSTR_VAL(new_value), allowed[i]) == 0) {
            is_allowed = true;
            break;
        }
    }

    if (!is_allowed) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Invalid value for GPU backend. Must be one of 'auto', 'cuda', 'rocm', or 'sycl'.");
        return FAILURE;
    }
    OnUpdateString(entry, new_value, mh_arg1, mh_arg2, mh_arg3);
    return SUCCESS;
}


PHP_INI_BEGIN()
    /* --- DataFrame Engine --- */
    STD_PHP_INI_ENTRY("quicpro.dataframe_enable", "1", PHP_INI_SYSTEM, OnUpdateBool, dataframe_enable, qp_high_perf_compute_ai_config_t, quicpro_high_perf_compute_ai_config)
    ZEND_INI_ENTRY_EX("quicpro.dataframe_memory_limit_mb", "1024", PHP_INI_SYSTEM, OnUpdateAiPositiveLong, NULL, NULL, NULL)
    STD_PHP_INI_ENTRY("quicpro.dataframe_string_interning_enable", "1", PHP_INI_SYSTEM, OnUpdateBool, dataframe_string_interning_enable, qp_high_perf_compute_ai_config_t, quicpro_high_perf_compute_ai_config)
    STD_PHP_INI_ENTRY("quicpro.dataframe_cpu_parallelism_default", "0", PHP_INI_SYSTEM, OnUpdateLong, dataframe_cpu_parallelism_default, qp_high_perf_compute_ai_config_t, quicpro_high_perf_compute_ai_config)

    /* --- General GPU Configuration --- */
    STD_PHP_INI_ENTRY("quicpro.gpu_bindings_enable", "0", PHP_INI_SYSTEM, OnUpdateBool, gpu_bindings_enable, qp_high_perf_compute_ai_config_t, quicpro_high_perf_compute_ai_config)
    ZEND_INI_ENTRY_EX("quicpro.gpu_default_backend", "auto", PHP_INI_SYSTEM, OnUpdateGpuBackend, &quicpro_high_perf_compute_ai_config.gpu_default_backend, NULL, NULL)
    STD_PHP_INI_ENTRY("quicpro.worker_gpu_affinity_map", "", PHP_INI_SYSTEM, OnUpdateString, worker_gpu_affinity_map, qp_high_perf_compute_ai_config_t, quicpro_high_perf_compute_ai_config)
    ZEND_INI_ENTRY_EX("quicpro.gpu_memory_preallocation_mb", "2048", PHP_INI_SYSTEM, OnUpdateAiPositiveLong, NULL, NULL, NULL)
    STD_PHP_INI_ENTRY("quicpro.gpu_p2p_enable", "1", PHP_INI_SYSTEM, OnUpdateBool, gpu_p2p_enable, qp_high_perf_compute_ai_config_t, quicpro_high_perf_compute_ai_config)
    STD_PHP_INI_ENTRY("quicpro.storage_enable_directstorage", "0", PHP_INI_SYSTEM, OnUpdateBool, storage_enable_directstorage, qp_high_perf_compute_ai_config_t, quicpro_high_perf_compute_ai_config)

    /* --- NVIDIA CUDA Specific Settings --- */
    STD_PHP_INI_ENTRY("quicpro.cuda_enable_tensor_cores", "1", PHP_INI_SYSTEM, OnUpdateBool, cuda_enable_tensor_cores, qp_high_perf_compute_ai_config_t, quicpro_high_perf_compute_ai_config)
    ZEND_INI_ENTRY_EX("quicpro.cuda_stream_pool_size", "4", PHP_INI_SYSTEM, OnUpdateAiPositiveLong, NULL, NULL, NULL)

    /* --- AMD ROCm Specific Settings --- */
    STD_PHP_INI_ENTRY("quicpro.rocm_enable_gfx_optimizations", "1", PHP_INI_SYSTEM, OnUpdateBool, rocm_enable_gfx_optimizations, qp_high_perf_compute_ai_config_t, quicpro_high_perf_compute_ai_config)

    /* --- Intel Arc (SYCL) Specific Settings --- */
    STD_PHP_INI_ENTRY("quicpro.arc_enable_xmx_optimizations", "1", PHP_INI_SYSTEM, OnUpdateBool, arc_enable_xmx_optimizations, qp_high_perf_compute_ai_config_t, quicpro_high_perf_compute_ai_config)
    STD_PHP_INI_ENTRY("quicpro.arc_video_acceleration_enable", "1", PHP_INI_SYSTEM, OnUpdateBool, arc_video_acceleration_enable, qp_high_perf_compute_ai_config_t, quicpro_high_perf_compute_ai_config)
PHP_INI_END()

void qp_config_high_perf_compute_and_ai_ini_register(void) { REGISTER_INI_ENTRIES(); }
void qp_config_high_perf_compute_and_ai_ini_unregister(void) { UNREGISTER_INI_ENTRIES(); }
