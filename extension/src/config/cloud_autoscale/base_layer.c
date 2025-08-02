/*
 * =========================================================================
 * FILENAME:   src/config/cloud_autoscale/base_layer.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    The mystery of life isn't a problem to solve, but a
 * reality to experience.
 *
 * PURPOSE:
 * This file provides the single, authoritative definition and memory
 * allocation for the `quicpro_cloud_autoscale_config` global variable.
 * =========================================================================
 */
#include "include/config/cloud_autoscale/base_layer.h"

/*
 * This is the actual definition and memory allocation for the module's
 * configuration structure.
 */
qp_cloud_autoscale_config_t quicpro_cloud_autoscale_config;
