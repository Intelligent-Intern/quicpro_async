/*
 * =========================================================================
 * FILENAME:   src/config/high_perf_compute_and_ai/default.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Easy money.
 *
 * PURPOSE:
 * This file implements the function that loads the hardcoded, default
 * values into the High-Performance Compute & AI configuration struct.
 * =========================================================================
 */

#include "include/config/high_perf_compute_and_ai/default.h"
#include "include/config/high_perf_compute_and_ai/base_layer.h"
#include "php.h"

/**
 * @brief Loads the hardcoded, default values into the module's config struct.
 */
void qp_config_high_perf_compute_and_ai_defaults_load(void)
{
    /* --- DataFrame Engine (CPU-based Analytics) --- */
    quicpro_high_perf_compute_ai_config.dataframe_enable = true;
    quicpro_high_perf_compute_ai_config.dataframe_memory_limit_mb = 1024;
    quicpro_high_perf_compute_ai_config.dataframe_string_interning_enable = true;
    quicpro_high_perf_compute_ai_config.dataframe_cpu_parallelism_default = 0; /* Auto-detect */

    /* --- General GPU Configuration --- */
    quicpro_high_perf_compute_ai_config.gpu_bindings_enable = false; /* Disabled by default for safety */
    quicpro_high_perf_compute_ai_config.gpu_default_backend = pestrdup("auto", 1);
    quicpro_high_perf_compute_ai_config.worker_gpu_affinity_map = pestrdup("", 1);
    quicpro_high_perf_compute_ai_config.gpu_memory_preallocation_mb = 2048;
    quicpro_high_perf_compute_ai_config.gpu_p2p_enable = true;
    quicpro_high_perf_compute_ai_config.storage_enable_directstorage = false;

    /* --- NVIDIA CUDA Specific Settings --- */
    quicpro_high_perf_compute_ai_config.cuda_enable_tensor_cores = true;
    quicpro_high_perf_compute_ai_config.cuda_stream_pool_size = 4;

    /* --- AMD ROCm Specific Settings --- */
    quicpro_high_perf_compute_ai_config.rocm_enable_gfx_optimizations = true;

    /* --- Intel Arc (SYCL) Specific Settings --- */
    quicpro_high_perf_compute_ai_config.arc_enable_xmx_optimizations = true;
    quicpro_high_perf_compute_ai_config.arc_video_acceleration_enable = true;
}
