/*
 * =========================================================================
 * FILENAME:   src/config/quic_transport/default.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    You were so preoccupied with whether or not you could,
 * you didn't stop to think if you should.
 *
 * PURPOSE:
 * This file implements the function that loads the hardcoded, default
 * values into the QUIC Transport configuration struct.
 * =========================================================================
 */

#include "include/config/quic_transport/default.h"
#include "include/config/quic_transport/base_layer.h"
#include "php.h"

/**
 * @brief Loads the hardcoded, default values into the module's config struct.
 */
void qp_config_quic_transport_defaults_load(void)
{
    /* --- Congestion Control & Pacing --- */
    quicpro_quic_transport_config.cc_algorithm = pestrdup("cubic", 1);
    quicpro_quic_transport_config.cc_initial_cwnd_packets = 32;
    quicpro_quic_transport_config.cc_min_cwnd_packets = 4;
    quicpro_quic_transport_config.cc_enable_hystart_plus_plus = true;
    quicpro_quic_transport_config.pacing_enable = true;
    quicpro_quic_transport_config.pacing_max_burst_packets = 10;

    /* --- Loss Recovery, ACK Management & Timers --- */
    quicpro_quic_transport_config.max_ack_delay_ms = 25;
    quicpro_quic_transport_config.ack_delay_exponent = 3;
    quicpro_quic_transport_config.pto_timeout_ms_initial = 1000;
    quicpro_quic_transport_config.pto_timeout_ms_max = 60000;
    quicpro_quic_transport_config.max_pto_probes = 5;
    quicpro_quic_transport_config.ping_interval_ms = 15000;

    /* --- Flow Control & Stream Limits --- */
    quicpro_quic_transport_config.initial_max_data = 10485760;
    quicpro_quic_transport_config.initial_max_stream_data_bidi_local = 1048576;
    quicpro_quic_transport_config.initial_max_stream_data_bidi_remote = 1048576;
    quicpro_quic_transport_config.initial_max_stream_data_uni = 1048576;
    quicpro_quic_transport_config.initial_max_streams_bidi = 100;
    quicpro_quic_transport_config.initial_max_streams_uni = 100;

    /* --- Protocol Features & Datagrams --- */
    quicpro_quic_transport_config.active_connection_id_limit = 8;
    quicpro_quic_transport_config.stateless_retry_enable = false;
    quicpro_quic_transport_config.grease_enable = true;
    quicpro_quic_transport_config.datagrams_enable = true;
    quicpro_quic_transport_config.dgram_recv_queue_len = 1024;
    quicpro_quic_transport_config.dgram_send_queue_len = 1024;
}
