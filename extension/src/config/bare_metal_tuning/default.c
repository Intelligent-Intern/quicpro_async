/*
 * =========================================================================
 * FILENAME:   src/config/bare_metal_tuning/default.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    I want more life, father.
 *
 * PURPOSE:
 * This file implements the function that loads the hardcoded, default
 * values into the bare metal tuning configuration struct.
 * =========================================================================
 */

#include "include/config/bare_metal_tuning/default.h"
#include "include/config/bare_metal_tuning/base_layer.h"
#include "php.h"

/**
 * @brief Loads the hardcoded, default values into the module's config struct.
 */
void qp_config_bare_metal_tuning_defaults_load(void)
{
    /* --- Low-Level I/O Engine --- */
    quicpro_bare_metal_config.io_engine_use_uring = true;
    quicpro_bare_metal_config.io_uring_sq_poll_ms = 0;
    quicpro_bare_metal_config.io_max_batch_read_packets = 64;
    quicpro_bare_metal_config.io_max_batch_write_packets = 64;

    /* --- Socket Buffers & Options --- */
    quicpro_bare_metal_config.socket_receive_buffer_size = 2097152; /* 2MB */
    quicpro_bare_metal_config.socket_send_buffer_size = 2097152; /* 2MB */
    quicpro_bare_metal_config.socket_enable_busy_poll_us = 0;
    quicpro_bare_metal_config.socket_enable_timestamping = true;

    /* --- CPU & NUMA Affinity --- */
    quicpro_bare_metal_config.io_thread_cpu_affinity = pestrdup("", 1);
    quicpro_bare_metal_config.io_thread_numa_node_policy = pestrdup("default", 1);
}
