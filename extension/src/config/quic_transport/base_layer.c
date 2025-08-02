/*
 * =========================================================================
 * FILENAME:   src/config/quic_transport/base_layer.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    God creates dinosaurs. God destroys dinosaurs. God creates man.
 * Man destroys God. Man creates dinosaurs.
 *
 * PURPOSE:
 * This file provides the single, authoritative definition and memory
 * allocation for the `quicpro_quic_transport_config` global variable.
 * =========================================================================
 */
#include "include/config/quic_transport/base_layer.h"

/*
 * This is the actual definition and memory allocation for the module's
 * configuration structure.
 */
qp_quic_transport_config_t quicpro_quic_transport_config;
