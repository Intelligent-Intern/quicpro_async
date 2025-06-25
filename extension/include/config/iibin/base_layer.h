/*
 * =========================================================================
 * FILENAME:   include/config/iibin/base_layer.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    The Guide is definitive. Reality is frequently inaccurate.
 *
 * PURPOSE:
 * This header file defines the core data structure for the
 * `iibin` (Intelligent, Instant Binary) serialization configuration module.
 *
 * ARCHITECTURE:
 * This struct holds the configuration values for the serialization engine
 * and its optional zero-copy I/O mechanism.
 * =========================================================================
 */
#ifndef QUICPRO_CONFIG_IIBIN_BASE_H
#define QUICPRO_CONFIG_IIBIN_BASE_H

#include "php.h"
#include <stdbool.h>

typedef struct _qp_iibin_config_t {
    /* --- IIBIN Serialization Engine --- */
    zend_long max_schema_fields;
    zend_long max_recursion_depth;
    bool string_interning_enable;

    /* --- Zero-Copy I/O via Shared Memory Buffers --- */
    bool use_shared_memory_buffers;
    zend_long default_buffer_size_kb;
    zend_long shm_total_memory_mb;
    char *shm_path;

} qp_iibin_config_t;

/* The single instance of this module's configuration data */
extern qp_iibin_config_t quicpro_iibin_config;

#endif /* QUICPRO_CONFIG_IIBIN_BASE_H */
