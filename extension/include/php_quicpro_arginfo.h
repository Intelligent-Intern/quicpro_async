/* extension/include/php_quicpro_arginfo.h
 *
 * Argument information for all PHP functions provided by the php-quicpro_async
 * extension.  These macros describe the expected parameter types, names,
 * and optionality to the Zend engine, enabling:
 *
 *   • Type checking at call time
 *   • Reflection of parameter names and types in userland
 *   • Generation of accurate stubs and documentation
 *
 * Each ZEND_BEGIN_ARG_INFO_EX block corresponds to one PHP_FUNCTION().
 * The entries inside list each parameter:
 *   – 0th argument: pass‑by‑value or pass‑by‑reference (0 = by value)
 *   – Parameter name as seen in PHP code
 *   – Expected type (IS_STRING, IS_LONG, IS_ARRAY, etc.)
 *   – Whether the parameter may be NULL (1 = nullable, 0 = required)
 */

#ifndef PHP_QUICPRO_ARGINFO_H
#define PHP_QUICPRO_ARGINFO_H

/*------------------------------------------------------------------------*/
/* quicpro_connect(string $host, int $port): resource                      */
/*------------------------------------------------------------------------*/
/*
 * Open a new QUIC session to the given host and port.
 *   $host:  DNS name or IP address of the server
 *   $port:  UDP port number (e.g. 443 for HTTPS)
 *
 * Returns a session resource on success, or throws on failure.
 */
ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_connect, 0, 0, 2)
    ZEND_ARG_TYPE_INFO(0, host, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, port, IS_LONG,   0)
ZEND_END_ARG_INFO()

/*------------------------------------------------------------------------*/
/* quicpro_close(resource|object $session): bool                           */
/*------------------------------------------------------------------------*/
/*
 * Gracefully close an existing QUIC session.
 *   $session:  The resource returned by quicpro_connect() or Quicpro\Session object
 *
 * Returns true on success, false if the resource/object was invalid.
 */
ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_close, 0, 0, 1)
    ZEND_ARG_INFO(0, session)
ZEND_END_ARG_INFO()

/*------------------------------------------------------------------------*/
/* quicpro_send_request(resource $session, string $path, array|null $headers, string|null $body): int */
/*------------------------------------------------------------------------*/
/*
 * Send an HTTP/3 request on the given session.
 *   $session:  The resource returned by quicpro_connect()
 *   $path:     The request target (e.g. "/index.html")
 *   $headers:  Optional associative array of additional headers
 *   $body:     Optional request body to send (for POST/PUT)
 *
 * Returns a numeric stream ID for the request, or throws on error.
 */
ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_send_request, 0, 0, 2)
    ZEND_ARG_INFO(0, session)
    ZEND_ARG_TYPE_INFO(0, path,    IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, headers, IS_ARRAY,  1)
    ZEND_ARG_TYPE_INFO(0, body,    IS_STRING, 1)
ZEND_END_ARG_INFO()

/*------------------------------------------------------------------------*/
/* quicpro_receive_response(resource $session, int $stream_id): string     */
/*------------------------------------------------------------------------*/
/*
 * Receive the full HTTP/3 response for a given stream.
 *   $session:   The resource returned by quicpro_connect()
 *   $stream_id: The stream ID returned by quicpro_send_request()
 *
 * Returns the response body as a string, or throws on error.
 */
ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_receive_response, 0, 0, 2)
    ZEND_ARG_INFO(0, session)
    ZEND_ARG_TYPE_INFO(0, stream_id, IS_LONG, 0)
ZEND_END_ARG_INFO()

/*------------------------------------------------------------------------*/
/* quicpro_poll(resource $session, int $timeout_ms): bool                  */
/*------------------------------------------------------------------------*/
/*
 * Perform one iteration of the event loop for the given session.
 *   $session:   The resource returned by quicpro_connect()
 *   $timeout_ms: Maximum time in milliseconds to block (negative = indefinite)
 *
 * Drains incoming packets, sends outgoing packets, and handles timeouts.
 * Returns true on success, false on invalid session.
 */
ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_poll, 0, 0, 2)
    ZEND_ARG_INFO(0, session)
    ZEND_ARG_TYPE_INFO(0, timeout_ms, IS_LONG, 0)
ZEND_END_ARG_INFO()

/*------------------------------------------------------------------------*/
/* quicpro_cancel_stream(resource $session, int $stream_id): bool          */
/*------------------------------------------------------------------------*/
/*
 * Abruptly shutdown a specific HTTP/3 stream for reading/writing.
 *   $session:   The resource returned by quicpro_connect()
 *   $stream_id: The stream ID to cancel
 *
 * Returns true on success, false on error or invalid stream.
 */
ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_cancel_stream, 0, 0, 2)
    ZEND_ARG_INFO(0, session)
    ZEND_ARG_TYPE_INFO(0, stream_id, IS_LONG, 0)
ZEND_END_ARG_INFO()

/*------------------------------------------------------------------------*/
/* quicpro_export_session_ticket(resource $session): string                 */
/*------------------------------------------------------------------------*/
/*
 * Export the latest TLS session ticket from an active QUIC session.
 *   $session: The resource returned by quicpro_connect()
 *
 * Returns a binary string containing the ticket, or an empty string
 * if no ticket is available yet.
 */
ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_export_session_ticket, 0, 0, 1)
    ZEND_ARG_INFO(0, session)
ZEND_END_ARG_INFO()

/*------------------------------------------------------------------------*/
/* quicpro_import_session_ticket(resource $session, string $ticket): bool   */
/*------------------------------------------------------------------------*/
/*
 * Import a previously exported TLS session ticket into a new session,
 * enabling 0‑RTT handshake resumption.
 *   $session: The resource returned by quicpro_connect()
 *   $ticket:  The binary ticket string returned by export_session_ticket()
 *
 * Returns true on success, false if ticket is invalid or rejected.
 */
ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_import_session_ticket, 0, 0, 2)
    ZEND_ARG_INFO(0, session)
    ZEND_ARG_TYPE_INFO(0, ticket, IS_STRING, 0)
ZEND_END_ARG_INFO()

/*------------------------------------------------------------------------*/
/* quicpro_set_ca_file(string $ca_file): bool                              */
/*------------------------------------------------------------------------*/
/*
 * Set the file path for the CA bundle to use in all future QUIC configs.
 *   $ca_file: Path to a PEM‑formatted CA certificate bundle
 *
 * Returns true on success.  Does not affect already‑open sessions.
 */
ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_set_ca_file, 0, 0, 1)
    ZEND_ARG_TYPE_INFO(0, ca_file, IS_STRING, 0)
ZEND_END_ARG_INFO()

/*------------------------------------------------------------------------*/
/* quicpro_set_client_cert(string $cert_pem, string $key_pem): bool        */
/*------------------------------------------------------------------------*/
/*
 * Set the client certificate and private key files for mutual TLS.
 *   $cert_pem: Path to the PEM‑encoded client certificate chain
 *   $key_pem:  Path to the PEM‑encoded private key
 *
 * Returns true on success.  Applies only to future new_config calls.
 */
ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_set_client_cert, 0, 0, 2)
    ZEND_ARG_TYPE_INFO(0, cert_pem, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, key_pem,  IS_STRING, 0)
ZEND_END_ARG_INFO()

/*------------------------------------------------------------------------*/
/* quicpro_get_last_error(): string                                        */
/*------------------------------------------------------------------------*/
/*
 * Retrieve the last extension‑level error message set by any function.
 *
 * Returns the error text, or an empty string if no error is pending.
 */
ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_get_last_error, 0, 0, 0)
ZEND_END_ARG_INFO()

/*------------------------------------------------------------------------*/
/* quicpro_get_stats(resource $session): array                             */
/*------------------------------------------------------------------------*/
/*
 * Return transport‑level statistics for the given session:
 *   pkt_rx, pkt_tx, lost, rtt_ns, cwnd
 *
 *   $session: The resource returned by quicpro_connect()
 *
 * Returns an associative array of metric names to integer values.
 */
ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_get_stats, 0, 0, 1)
    ZEND_ARG_INFO(0, session)
ZEND_END_ARG_INFO()

/*------------------------------------------------------------------------*/
/* quicpro_version(): string                                               */
/*------------------------------------------------------------------------*/
/*
 * Return the version string of the loaded php-quicpro_async extension.
 * This corresponds to PHP_QUICPRO_VERSION from php_quicpro.h.
 */
ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_version, 0, 0, 0)
ZEND_END_ARG_INFO()

#endif /* PHP_QUICPRO_ARGINFO_H */
