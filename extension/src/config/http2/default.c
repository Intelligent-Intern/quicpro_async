/*
 * =========================================================================
 * FILENAME:   src/config/http2/default.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    What happens to us in the future? Do we become assholes or something?
 *
 * PURPOSE:
 * This file implements the function that loads the hardcoded, default
 * values into the HTTP/2 configuration struct.
 * =========================================================================
 */

#include "include/config/http2/default.h"
#include "include/config/http2/base_layer.h"
#include "php.h"

/**
 * @brief Loads the hardcoded, default values into the module's config struct.
 */
void qp_config_http2_defaults_load(void)
{
    quicpro_http2_config.enable = true;
    quicpro_http2_config.initial_window_size = 65535; /* Default per RFC 7540 */
    quicpro_http2_config.max_concurrent_streams = 100;
    quicpro_http2_config.max_header_list_size = 0; /* 0 means unlimited by default */
    quicpro_http2_config.enable_push = true;
    quicpro_http2_config.max_frame_size = 16384; /* Default per RFC 7540 */
}
