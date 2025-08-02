/*
 * =========================================================================
 * FILENAME:   src/config/quic_transport/config.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    That is one big pile of shit.
 *
 * PURPOSE:
 * This file implements the logic for applying runtime configuration
 * changes to the QUIC Transport module.
 * =========================================================================
 */

#include "include/config/quic_transport/config.h"
#include "include/config/quic_transport/base_layer.h"
#include "include/quicpro_globals.h"

/* Centralized validation helpers */
#include "include/validation/config_param/validate_bool.h"
#include "include/validation/config_param/validate_positive_long.h"
#include "include/validation/config_param/validate_non_negative_long.h"
#include "include/validation/config_param/validate_string_from_allowlist.h"

#include "php.h"
#include <ext/spl/spl_exceptions.h>

int qp_config_quic_transport_apply_userland_config(zval *config_arr)
{
    zval *value;
    zend_string *key;

    if (!quicpro_globals.is_userland_override_allowed) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Configuration override from userland is disabled by system administrator.");
        return FAILURE;
    }

    if (Z_TYPE_P(config_arr) != IS_ARRAY) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Configuration must be provided as an array.");
        return FAILURE;
    }

    ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(config_arr), key, value) {
        if (!key) continue;

        if (zend_string_equals_literal(key, "cc_algorithm")) {
            const char *allowed[] = {"cubic", "bbr", NULL};
            if (qp_validate_string_from_allowlist(value, allowed, &quicpro_quic_transport_config.cc_algorithm) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "cc_initial_cwnd_packets")) {
            if (qp_validate_positive_long(value, &quicpro_quic_transport_config.cc_initial_cwnd_packets) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "cc_min_cwnd_packets")) {
            if (qp_validate_positive_long(value, &quicpro_quic_transport_config.cc_min_cwnd_packets) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "cc_enable_hystart_plus_plus")) {
            if (qp_validate_bool(value) != SUCCESS) return FAILURE;
            quicpro_quic_transport_config.cc_enable_hystart_plus_plus = zend_is_true(value);
        } else if (zend_string_equals_literal(key, "pacing_enable")) {
            if (qp_validate_bool(value) != SUCCESS) return FAILURE;
            quicpro_quic_transport_config.pacing_enable = zend_is_true(value);
        } else if (zend_string_equals_literal(key, "pacing_max_burst_packets")) {
            if (qp_validate_positive_long(value, &quicpro_quic_transport_config.pacing_max_burst_packets) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "max_ack_delay_ms")) {
            if (qp_validate_non_negative_long(value, &quicpro_quic_transport_config.max_ack_delay_ms) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "ack_delay_exponent")) {
            if (qp_validate_non_negative_long(value, &quicpro_quic_transport_config.ack_delay_exponent) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "pto_timeout_ms_initial")) {
            if (qp_validate_positive_long(value, &quicpro_quic_transport_config.pto_timeout_ms_initial) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "pto_timeout_ms_max")) {
            if (qp_validate_positive_long(value, &quicpro_quic_transport_config.pto_timeout_ms_max) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "max_pto_probes")) {
            if (qp_validate_positive_long(value, &quicpro_quic_transport_config.max_pto_probes) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "ping_interval_ms")) {
            if (qp_validate_non_negative_long(value, &quicpro_quic_transport_config.ping_interval_ms) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "initial_max_data")) {
            if (qp_validate_positive_long(value, &quicpro_quic_transport_config.initial_max_data) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "initial_max_stream_data_bidi_local")) {
            if (qp_validate_positive_long(value, &quicpro_quic_transport_config.initial_max_stream_data_bidi_local) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "initial_max_stream_data_bidi_remote")) {
            if (qp_validate_positive_long(value, &quicpro_quic_transport_config.initial_max_stream_data_bidi_remote) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "initial_max_stream_data_uni")) {
            if (qp_validate_positive_long(value, &quicpro_quic_transport_config.initial_max_stream_data_uni) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "initial_max_streams_bidi")) {
            if (qp_validate_positive_long(value, &quicpro_quic_transport_config.initial_max_streams_bidi) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "initial_max_streams_uni")) {
            if (qp_validate_positive_long(value, &quicpro_quic_transport_config.initial_max_streams_uni) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "active_connection_id_limit")) {
            if (qp_validate_positive_long(value, &quicpro_quic_transport_config.active_connection_id_limit) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "stateless_retry_enable")) {
            if (qp_validate_bool(value) != SUCCESS) return FAILURE;
            quicpro_quic_transport_config.stateless_retry_enable = zend_is_true(value);
        } else if (zend_string_equals_literal(key, "grease_enable")) {
            if (qp_validate_bool(value) != SUCCESS) return FAILURE;
            quicpro_quic_transport_config.grease_enable = zend_is_true(value);
        } else if (zend_string_equals_literal(key, "datagrams_enable")) {
            if (qp_validate_bool(value) != SUCCESS) return FAILURE;
            quicpro_quic_transport_config.datagrams_enable = zend_is_true(value);
        } else if (zend_string_equals_literal(key, "dgram_recv_queue_len")) {
            if (qp_validate_positive_long(value, &quicpro_quic_transport_config.dgram_recv_queue_len) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "dgram_send_queue_len")) {
            if (qp_validate_positive_long(value, &quicpro_quic_transport_config.dgram_send_queue_len) != SUCCESS) return FAILURE;
        }

    } ZEND_HASH_FOREACH_END();

    return SUCCESS;
}
