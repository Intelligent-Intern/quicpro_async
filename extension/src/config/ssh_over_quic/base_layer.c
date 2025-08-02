/*
 * =========================================================================
 * FILENAME:   src/config/ssh_over_quic/base_layer.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    "You move like they do. I've never seen anyone move that fast..."
 *
 * PURPOSE:
 *   Provides the single, authoritative definition and memory allocation for
 *   the `quicpro_ssh_quic_config` global configuration structure that backs
 *   the SSH‑over‑QUIC gateway module.
 * =========================================================================
 */

#include "include/config/ssh_over_quic/base_layer.h"

/*
 * Global, runtime instance of the configuration struct.
 */
qp_ssh_quic_config_t quicpro_ssh_quic_config;
