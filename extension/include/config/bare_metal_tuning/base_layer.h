/*
 * =========================================================================
 * FILENAME:   include/config/bare_metal_tuning/base_layer.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    All those moments will be lost in time, like tears in rain.
 *
 * PURPOSE:
 * This header file defines the core data structure for the
 * `bare_metal_tuning` configuration module.
 *
 * ARCHITECTURE:
 * This struct holds the expert-level configuration values for direct
 * interaction with the host operating system's kernel and networking
 * stack after they have been loaded.
 * =========================================================================
 */
#ifndef QUICPRO_CONFIG_BARE_METAL_BASE_H
#define QUICPRO_CONFIG_BARE_METAL_BASE_H

#include "php.h"
#include <stdbool.h>

typedef struct _qp_bare_metal_config_t {
    /* --- Low-Level I/O Engine --- */
    bool io_engine_use_uring;
    zend_long io_uring_sq_poll_ms;
    zend_long io_max_batch_read_packets;
    zend_long io_max_batch_write_packets;

    /* --- Socket Buffers & Options --- */
    zend_long socket_receive_buffer_size;
    zend_long socket_send_buffer_size;
    zend_long socket_enable_busy_poll_us;
    bool socket_enable_timestamping;

    /* --- CPU & NUMA Affinity --- */
    char *io_thread_cpu_affinity;
    char *io_thread_numa_node_policy;

} qp_bare_metal_config_t;

/* The single instance of this module's configuration data */
extern qp_bare_metal_config_t quicpro_bare_metal_config;

#endif /* QUICPRO_CONFIG_BARE_METAL_BASE_H */
