/*
 * =========================================================================
 * FILENAME:   include/config/high_perf_compute_and_ai/base_layer.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    I'll be back.
 *
 * PURPOSE:
 * This header file defines the core data structure for the
 * `high_perf_compute_and_ai` configuration module.
 *
 * ARCHITECTURE:
 * This struct holds the configuration values for the native DataFrame and
 * GPU compute capabilities after they have been loaded.
 * =========================================================================
 */
#ifndef QUICPRO_CONFIG_HIGH_PERF_COMPUTE_AI_BASE_H
#define QUICPRO_CONFIG_HIGH_PERF_COMPUTE_AI_BASE_H

#include "php.h"
#include <stdbool.h>

typedef struct _qp_high_perf_compute_ai_config_t {
    /* --- DataFrame Engine (CPU-based Analytics) --- */
    bool dataframe_enable;
    zend_long dataframe_memory_limit_mb;
    bool dataframe_string_interning_enable;
    zend_long dataframe_cpu_parallelism_default;

    /* --- General GPU Configuration --- */
    bool gpu_bindings_enable;
    char *gpu_default_backend;
    char *worker_gpu_affinity_map;
    zend_long gpu_memory_preallocation_mb;
    bool gpu_p2p_enable;
    bool storage_enable_directstorage;

    /* --- NVIDIA CUDA Specific Settings --- */
    bool cuda_enable_tensor_cores;
    zend_long cuda_stream_pool_size;

    /* --- AMD ROCm Specific Settings --- */
    bool rocm_enable_gfx_optimizations;

    /* --- Intel Arc (SYCL) Specific Settings --- */
    bool arc_enable_xmx_optimizations;
    bool arc_video_acceleration_enable;

} qp_high_perf_compute_ai_config_t;

/* The single instance of this module's configuration data */
extern qp_high_perf_compute_ai_config_t quicpro_high_perf_compute_ai_config;

#endif /* QUICPRO_CONFIG_HIGH_PERF_COMPUTE_AI_BASE_H */
