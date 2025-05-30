/* include/config.h */
// ... (existing comments about overview) ...

#ifndef QUICPRO_CONFIG_H
#define QUICPRO_CONFIG_H

#include <php.h>
#include <quiche.h>

/*
 * quicpro_cfg_t
 * -------------
 * Resource wrapper for quiche_config*. The 'frozen' flag is set after
 * first use in a connection (e.g., via quicpro_mcp_connect) to enforce
 * immutability and prevent race conditions in multi-worker setups.
 * This struct is allocated and its lifetime managed by PHP's resource system.
 */
typedef struct {
    quiche_config *cfg;    /* Underlying QUIC/TLS config handle from libquiche */
    zend_bool      frozen; /* Boolean flag, set to true after first use */

    /*
     * NOTE: Most configuration options are applied directly to the `quiche_config *cfg`.
     * This struct primarily holds the libquiche config pointer and its frozen state.
     * Additional options not directly supported by `quiche_config` but relevant for
     * connection setup (e.g., default H3 settings to apply when an H3 config is made
     * from this QUIC config) could theoretically be stored here if they need to be
     * part of this reusable config object. However, for simplicity and direct mapping,
     * such extended options are often better passed to the connect function itself.
     * This structure focuses on options directly translatable to quiche_config settings.
     */
} quicpro_cfg_t;

/*
 * qp_fetch_cfg
 * (existing helper function)
 */
static inline quicpro_cfg_t *qp_fetch_cfg(zval *zcfg)
{
    extern int le_quicpro_cfg; // Resource type ID, defined in a .c file
    return (quicpro_cfg_t *) zend_fetch_resource_ex(
        zcfg, "quicpro_cfg", le_quicpro_cfg
    );
}

/*
 * quicpro_cfg_mark_frozen
 * (existing helper function)
 */
void quicpro_cfg_mark_frozen(zval *zcfg);

/*
 * PHP API Functions exposed by this module (config.c)
 * ----------------------------------------------------
 * The primary function is `quicpro_new_config`, which accepts an associative array
 * of options to configure the underlying `quiche_config` object.
 *
 * See documentation or comments in `src/config.c` (where options are parsed)
 * for the full list of supported keys in the $options array for `quicpro_new_config`.
 * Key categories include:
 *
 * I. TLS Configuration:
 * - 'application_protocols' (array of strings, e.g., ['h3', 'your-mcp-protocol/1.0'])
 * - 'verify_peer' (bool, default: true) - Enable/disable peer certificate verification.
 * - 'verify_depth' (int) - Set peer verification depth.
 * - 'ca_file' (string) - Path to CA bundle file for peer verification.
 * - 'ca_path' (string) - Path to directory containing CA certificates.
 * - 'cert_file' (string) - Path to PEM-encoded public certificate chain file (for server or client auth).
 * - 'key_file' (string) - Path to PEM-encoded private key file.
 * - 'ticket_key_file' (string) - For server: path to session ticket encryption key file.
 * - 'ciphers_tls13' (string) - OpenSSL-style string for TLS 1.3 ciphers (e.g., "TLS_AES_128_GCM_SHA256:TLS_CHACHA20_POLY1305_SHA256").
 * - 'curves' (string) - OpenSSL-style string for supported elliptic curves (e.g., "P-256:X25519").
 * - 'enable_early_data' (bool, default: false for client) - Enable 0-RTT.
 *
 * II. QUIC Protocol Configuration:
 * - 'max_idle_timeout_ms' (int) - Max idle time in milliseconds before connection closes.
 * - 'max_udp_payload_size' (int) - Max UDP payload size for sending. (quiche has send/recv separate)
 * - 'initial_max_data' (int) - Connection-level initial flow control limit.
 * - 'initial_max_stream_data_bidi_local' (int) - Initial flow control for locally-initiated bidi streams.
 * - 'initial_max_stream_data_bidi_remote' (int) - Initial flow control for remotely-initiated bidi streams.
 * - 'initial_max_stream_data_uni' (int) - Initial flow control for unidirectional streams.
 * - 'initial_max_streams_bidi' (int) - Max concurrent bidirectional streams.
 * - 'initial_max_streams_uni' (int) - Max concurrent unidirectional streams.
 * - 'ack_delay_exponent' (int)
 * - 'max_ack_delay_ms' (int)
 * - 'active_connection_id_limit' (int) - Max active connection IDs peer can supply.
 * - 'stateless_retry' (bool, for server-side, default: false) - Enable stateless retry.
 * - 'grease_level' (int, 0=off, 1=low, 2=high, default: based on quiche) - QUIC grease level.
 * - 'enable_datagrams' (bool, default: false) - Enable QUIC datagram support (RFC 9221).
 * - 'dgram_recv_queue_len' (int, if enable_datagrams) - Max unacknowledged incoming datagrams.
 * - 'dgram_send_queue_len' (int, if enable_datagrams) - Max outgoing datagrams buffered.
 *
 * III. Congestion Control & Pacing:
 * - 'congestion_control_algorithm' (string, e.g., "cubic", "reno", "bbr" - if supported by quiche build)
 * - 'enable_hystart' (bool, default: true for Cubic) - Enable Hystart for congestion control.
 * - 'enable_pacing' (bool, default: true) - Enable packet pacing.
 * - 'max_pacing_rate_bps' (int, bits per second) - Max pacing rate.
 * - 'disable_congestion_control_for_testing' (bool, default: false) - DANGER: Only for highly controlled test environments.
 *
 * IV. HTTP/3 Specific Defaults (these are applied when an H3 config is created from this QUIC config):
 * - 'h3_max_header_list_size' (int) - Max size of encoded HTTP header list.
 * - 'h3_qpack_max_table_capacity' (int) - QPACK dynamic table capacity.
 * - 'h3_qpack_blocked_streams' (int) - Max QPACK blocked streams.
 *
 * Note: Options related to socket behavior (like zero_copy, mmap, CPU affinity) or
 * application-level timeouts (connect_timeout_ms, per-request timeouts) are typically
 * passed to the `quicpro_mcp_connect` function's options array, as they are not
 * part of the `quiche_config` object itself but influence how the connection resource
 * is managed.
 */
PHP_FUNCTION(quicpro_new_config);
PHP_FUNCTION(quicpro_config_set_ca_file);     // For mutating non-frozen config
PHP_FUNCTION(quicpro_config_set_client_cert); // For mutating non-frozen config
PHP_FUNCTION(quicpro_config_export);          // To view current (non-sensitive) settings

#endif /* QUICPRO_CONFIG_H */