/*
 * =========================================================================
 * FILENAME:   src/config/security_and_traffic/base_layer.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    You know the rules, and so do I...
 *
 * PURPOSE:
 * This file provides the single, authoritative definition and memory
 * allocation for the `quicpro_security_config` global variable, which is
 * declared as `extern` in `base_layer.h`.
 * =========================================================================
 */
#include "include/config/security_and_traffic/base_layer.h"

/*
 * This is the actual definition and memory allocation for the module's
 * configuration structure.
 */
qp_security_config_t quicpro_security_config;
