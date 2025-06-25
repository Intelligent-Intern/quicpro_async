/*
 * =========================================================================
 * FILENAME:   src/config/app_http3_websockets_webtransport/base_layer.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    The Puppet Master: We have been subordinate to our
 * limitations until now.
 *
 * PURPOSE:
 * This file provides the single, authoritative definition and memory
 * allocation for the `quicpro_app_protocols_config` global variable.
 * =========================================================================
 */
#include "include/config/app_http3_websockets_webtransport/base_layer.h"

/*
 * This is the actual definition and memory allocation for the module's
 * configuration structure.
 */
qp_app_protocols_config_t quicpro_app_protocols_config;
