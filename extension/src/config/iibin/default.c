/*
 * =========================================================================
 * FILENAME:   src/config/iibin/default.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    We have normality. I repeat, we have normality. Anything
 * you still can't cope with is therefore your own problem.
 *
 * PURPOSE:
 * This file implements the function that loads the hardcoded, default
 * values into the IIBIN serialization configuration struct.
 * =========================================================================
 */

#include "include/config/iibin/default.h"
#include "include/config/iibin/base_layer.h"
#include "php.h"

/**
 * @brief Loads the hardcoded, default values into the module's config struct.
 */
void qp_config_iibin_defaults_load(void)
{
    /* --- IIBIN Serialization Engine --- */
    quicpro_iibin_config.max_schema_fields = 256;
    quicpro_iibin_config.max_recursion_depth = 32;
    quicpro_iibin_config.string_interning_enable = true;

    /* --- Zero-Copy I/O via Shared Memory Buffers --- */
    quicpro_iibin_config.use_shared_memory_buffers = false; /* Opt-in feature */
    quicpro_iibin_config.default_buffer_size_kb = 64;
    quicpro_iibin_config.shm_total_memory_mb = 256;
    quicpro_iibin_config.shm_path = pestrdup("/quicpro_io_shm", 1);
}
