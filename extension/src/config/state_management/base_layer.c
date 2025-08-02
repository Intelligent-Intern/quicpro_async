/*
 * =========================================================================
 * FILENAME:   src/config/state_management/base_layer.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    "The future is already here — it's just not evenly distributed."
 *
 * PURPOSE:
 *   Provides the single, authoritative definition and memory allocation for
 *   the `quicpro_state_mgmt_config` global variable used by the State-
 *   Management configuration module.
 * =========================================================================
 */

#include "include/config/state_management/base_layer.h"

/*
 * This is the actual definition and zero-initialisation of the module-wide
 * configuration structure.  It is intentionally kept in a dedicated TU so
 * that it is linked exactly once across the entire extension.
 */
qp_state_mgmt_config_t quicpro_state_mgmt_config = {0};
