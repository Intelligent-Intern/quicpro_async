/*
 * =========================================================================
 * FILENAME:   include/config/quic_transport/base_layer.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Life finds a way.
 *
 * PURPOSE:
 * This header file defines the core data structure for the
 * `quic_transport` configuration module.
 *
 * ARCHITECTURE:
 * This struct holds the expert-level configuration values for tuning the
 * raw behavior of the QUIC protocol itself.
 * =========================================================================
 */
#ifndef QUICPRO_CONFIG_QUIC_TRANSPORT_BASE_H
#define QUICPRO_CONFIG_QUIC_TRANSPORT_BASE_H

#include "php.h"
#include <stdbool.h>

typedef struct _qp_quic_transport_config_t {
    /* --- Congestion Control & Pacing --- */
    char *cc_algorithm;
    zend_long cc_initial_cwnd_packets;
    zend_long cc_min_cwnd_packets;
    bool cc_enable_hystart_plus_plus;
    bool pacing_enable;
    zend_long pacing_max_burst_packets;

    /* --- Loss Recovery, ACK Management & Timers --- */
    zend_long max_ack_delay_ms;
    zend_long ack_delay_exponent;
    zend_long pto_timeout_ms_initial;
    zend_long pto_timeout_ms_max;
    zend_long max_pto_probes;
    zend_long ping_interval_ms;

    /* --- Flow Control & Stream Limits --- */
    zend_long initial_max_data;
    zend_long initial_max_stream_data_bidi_local;
    zend_long initial_max_stream_data_bidi_remote;
    zend_long initial_max_stream_data_uni;
    zend_long initial_max_streams_bidi;
    zend_long initial_max_streams_uni;

    /* --- Protocol Features & Datagrams --- */
    zend_long active_connection_id_limit;
    bool stateless_retry_enable;
    bool grease_enable;
    bool datagrams_enable;
    zend_long dgram_recv_queue_len;
    zend_long dgram_send_queue_len;

} qp_quic_transport_config_t;

/* The single instance of this module's configuration data */
extern qp_quic_transport_config_t quicpro_quic_transport_config;

#endif /* QUICPRO_CONFIG_QUIC_TRANSPORT_BASE_H */
