/*
 * =========================================================================
 * FILENAME:   src/config/cluster_and_process/base_layer.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    There is no question about it.
 *
 * PURPOSE:
 * This file provides the single, authoritative definition and memory
 * allocation for the `quicpro_cluster_config` global variable, which is
 * declared as `extern` in `base_layer.h`.
 *
 * ARCHITECTURE:
 * In C, an `extern` variable declared in a header file must be defined
 * exactly once in a single source file across the entire project. This
 * file fulfills that requirement for this specific module's configuration
 * struct, ensuring a single, shared instance.
 * =========================================================================
 */
#include "include/config/cluster_and_process/base_layer.h"

/*
 * This is the actual definition and memory allocation for the module's
 * configuration structure.
 */
qp_cluster_config_t quicpro_cluster_config;
