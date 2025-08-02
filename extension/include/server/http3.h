/* extension/include/server/http3.h */

#ifndef QUICPRO_SERVER_HTTP3_H
#define QUICPRO_SERVER_HTTP3_H

#include <php.h>

/**
 * @file extension/include/server/http3.h
 * @brief Public API declarations for the Quicpro HTTP/3 Server.
 *
 * This header defines the primary interface for creating a native HTTP/3
 * server. HTTP/3 runs over the QUIC protocol on top of UDP, offering
 * significant performance advantages over its TCP-based predecessors,
 * including reduced head-of-line blocking and seamless connection migration.
 *
 * The `quicpro_http3_server_listen` function is the entry point for leveraging
 * these capabilities, enabling the creation of next-generation web services.
 */

/**
 * @brief Initializes and starts an HTTP/3 server listener on a specified address and port.
 *
 * This function sets up a UDP server specifically designed to handle the
 * QUIC transport protocol and the HTTP/3 application protocol. It utilizes
 * Quicpro's highly optimized, C-native QUIC stack for maximum performance,
 * concurrency, and resilience against challenging network conditions.
 *
 * A valid TLS 1.3 configuration is mandatory, as QUIC requires encryption.
 * The server will automatically handle the 'h3' ALPN negotiation.
 *
 * @param host_str The host address or IP on which the server should listen (e.g., "0.0.0.0").
 * @param host_len The length of the `host_str`.
 * @param port The UDP port number for the listener (e.g., 443).
 * @param config_resource A PHP resource representing a `Quicpro\Config` object. It must
 * contain a valid TLS configuration (`cert_file`, `key_file`) and should
 * specify 'h3' in its `alpn` list. QUIC-specific transport parameters
 * can also be provided here.
 * @param request_handler_callable A unified PHP callable that will be invoked for each
 * incoming HTTP/3 request.
 * @return TRUE on successful server initialization.
 * FALSE on failure, throwing a `Quicpro\Exception\ServerException` or a
 * more specific exception (e.g., `Quicpro\Exception\QuicException`) if
 * the UDP socket cannot be bound or the configuration is invalid.
 */
PHP_FUNCTION(quicpro_http3_server_listen);

#endif // QUICPRO_SERVER_HTTP3_H
