/*
 * =========================================================================
 * FILENAME:   src/config/quic_transport/ini.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Dodgson! Dodgson! We've got Dodgson here! See? Nobody cares.
 *
 * PURPOSE:
 * This file implements the registration, parsing, and validation of all
 * `php.ini` settings for the QUIC Transport module.
 * =========================================================================
 */

#include "include/config/quic_transport/ini.h"
#include "include/config/quic_transport/base_layer.h"
#include "php.h"
#include <zend_ini.h>
#include <ext/spl/spl_exceptions.h>

/* Custom OnUpdate handler for numeric values that must be strictly positive. */
static ZEND_INI_MH(OnUpdateQuicPositiveLong)
{
    zend_long val = zend_atol(ZSTR_VAL(new_value), ZSTR_LEN(new_value));
    if (val <= 0) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Invalid value for a QUIC transport directive. A positive integer is required.");
        return FAILURE;
    }
    /* This generic handler relies on the mh_arg1 pointing to the correct struct member */
    *(zend_long*)mh_arg1 = val;
    return SUCCESS;
}

/* Custom OnUpdate handler for numeric values that can be zero or positive. */
static ZEND_INI_MH(OnUpdateQuicNonNegativeLong)
{
    zend_long val = zend_atol(ZSTR_VAL(new_value), ZSTR_LEN(new_value));
    if (val < 0) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Invalid value for a QUIC transport directive. A non-negative integer is required.");
        return FAILURE;
    }
    *(zend_long*)mh_arg1 = val;
    return SUCCESS;
}

/* Custom OnUpdate handler for the ACK delay exponent (specific range 0-20). */
static ZEND_INI_MH(OnUpdateAckDelayExponent)
{
    zend_long val = zend_atol(ZSTR_VAL(new_value), ZSTR_LEN(new_value));
    if (val < 0 || val > 20) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Invalid value for ACK delay exponent. Must be an integer between 0 and 20.");
        return FAILURE;
    }
    quicpro_quic_transport_config.ack_delay_exponent = val;
    return SUCCESS;
}

/* Custom OnUpdate handler for the CC algorithm string */
static ZEND_INI_MH(OnUpdateCcAlgorithm)
{
    const char *allowed[] = {"cubic", "bbr", NULL};
    bool is_allowed = false;
    for (int i = 0; allowed[i] != NULL; i++) {
        if (strcasecmp(ZSTR_VAL(new_value), allowed[i]) == 0) {
            is_allowed = true;
            break;
        }
    }
    if (!is_allowed) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Invalid value for CC algorithm. Must be 'cubic' or 'bbr'.");
        return FAILURE;
    }
    OnUpdateString(entry, new_value, mh_arg1, mh_arg2, mh_arg3);
    return SUCCESS;
}


PHP_INI_BEGIN()
    ZEND_INI_ENTRY_EX("quicpro.transport_cc_algorithm", "cubic", PHP_INI_SYSTEM, OnUpdateCcAlgorithm, &quicpro_quic_transport_config.cc_algorithm, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.transport_cc_initial_cwnd_packets", "32", PHP_INI_SYSTEM, OnUpdateQuicPositiveLong, &quicpro_quic_transport_config.cc_initial_cwnd_packets, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.transport_cc_min_cwnd_packets", "4", PHP_INI_SYSTEM, OnUpdateQuicPositiveLong, &quicpro_quic_transport_config.cc_min_cwnd_packets, NULL, NULL)
    STD_PHP_INI_ENTRY("quicpro.transport_cc_enable_hystart_plus_plus", "1", PHP_INI_SYSTEM, OnUpdateBool, cc_enable_hystart_plus_plus, qp_quic_transport_config_t, quicpro_quic_transport_config)
    STD_PHP_INI_ENTRY("quicpro.transport_pacing_enable", "1", PHP_INI_SYSTEM, OnUpdateBool, pacing_enable, qp_quic_transport_config_t, quicpro_quic_transport_config)
    ZEND_INI_ENTRY_EX("quicpro.transport_pacing_max_burst_packets", "10", PHP_INI_SYSTEM, OnUpdateQuicPositiveLong, &quicpro_quic_transport_config.pacing_max_burst_packets, NULL, NULL)

    ZEND_INI_ENTRY_EX("quicpro.transport_max_ack_delay_ms", "25", PHP_INI_SYSTEM, OnUpdateQuicNonNegativeLong, &quicpro_quic_transport_config.max_ack_delay_ms, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.transport_ack_delay_exponent", "3", PHP_INI_SYSTEM, OnUpdateAckDelayExponent, NULL, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.transport_pto_timeout_ms_initial", "1000", PHP_INI_SYSTEM, OnUpdateQuicPositiveLong, &quicpro_quic_transport_config.pto_timeout_ms_initial, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.transport_pto_timeout_ms_max", "60000", PHP_INI_SYSTEM, OnUpdateQuicPositiveLong, &quicpro_quic_transport_config.pto_timeout_ms_max, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.transport_max_pto_probes", "5", PHP_INI_SYSTEM, OnUpdateQuicPositiveLong, &quicpro_quic_transport_config.max_pto_probes, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.transport_ping_interval_ms", "15000", PHP_INI_SYSTEM, OnUpdateQuicNonNegativeLong, &quicpro_quic_transport_config.ping_interval_ms, NULL, NULL)

    ZEND_INI_ENTRY_EX("quicpro.transport_initial_max_data", "10485760", PHP_INI_SYSTEM, OnUpdateQuicPositiveLong, &quicpro_quic_transport_config.initial_max_data, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.transport_initial_max_stream_data_bidi_local", "1048576", PHP_INI_SYSTEM, OnUpdateQuicPositiveLong, &quicpro_quic_transport_config.initial_max_stream_data_bidi_local, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.transport_initial_max_stream_data_bidi_remote", "1048576", PHP_INI_SYSTEM, OnUpdateQuicPositiveLong, &quicpro_quic_transport_config.initial_max_stream_data_bidi_remote, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.transport_initial_max_stream_data_uni", "1048576", PHP_INI_SYSTEM, OnUpdateQuicPositiveLong, &quicpro_quic_transport_config.initial_max_stream_data_uni, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.transport_initial_max_streams_bidi", "100", PHP_INI_SYSTEM, OnUpdateQuicPositiveLong, &quicpro_quic_transport_config.initial_max_streams_bidi, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.transport_initial_max_streams_uni", "100", PHP_INI_SYSTEM, OnUpdateQuicPositiveLong, &quicpro_quic_transport_config.initial_max_streams_uni, NULL, NULL)

    ZEND_INI_ENTRY_EX("quicpro.transport_active_connection_id_limit", "8", PHP_INI_SYSTEM, OnUpdateQuicPositiveLong, &quicpro_quic_transport_config.active_connection_id_limit, NULL, NULL)
    STD_PHP_INI_ENTRY("quicpro.transport_stateless_retry_enable", "0", PHP_INI_SYSTEM, OnUpdateBool, stateless_retry_enable, qp_quic_transport_config_t, quicpro_quic_transport_config)
    STD_PHP_INI_ENTRY("quicpro.transport_grease_enable", "1", PHP_INI_SYSTEM, OnUpdateBool, grease_enable, qp_quic_transport_config_t, quicpro_quic_transport_config)
    STD_PHP_INI_ENTRY("quicpro.transport_datagrams_enable", "1", PHP_INI_SYSTEM, OnUpdateBool, datagrams_enable, qp_quic_transport_config_t, quicpro_quic_transport_config)
    ZEND_INI_ENTRY_EX("quicpro.transport_dgram_recv_queue_len", "1024", PHP_INI_SYSTEM, OnUpdateQuicPositiveLong, &quicpro_quic_transport_config.dgram_recv_queue_len, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.transport_dgram_send_queue_len", "1024", PHP_INI_SYSTEM, OnUpdateQuicPositiveLong, &quicpro_quic_transport_config.dgram_send_queue_len, NULL, NULL)
PHP_INI_END()

void qp_config_quic_transport_ini_register(void) { REGISTER_INI_ENTRIES(); }
void qp_config_quic_transport_ini_unregister(void) { UNREGISTER_INI_ENTRIES(); }
