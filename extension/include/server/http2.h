/* extension/include/server/http2.h */

#ifndef QUICPRO_SERVER_HTTP2_H
#define QUICPRO_SERVER_HTTP2_H

#include <php.h>

/**
 * @file extension/include/server/http2.h
 * @brief Public API declarations for the Quicpro HTTP/2 Server.
 *
 * This header defines the PHP-callable functions for establishing and
 * managing an HTTP/2 server. It provides the necessary interface to handle
 * HTTP/2 traffic over TCP, typically secured with TLS and negotiated via ALPN.
 *
 * The `quicpro_http2_server_listen` function is the primary entry point,
 * encapsulating the setup of a high-performance, concurrent server capable
 * of handling multiple streams within each connection, a core feature of HTTP/2.
 */

/**
 * @brief Initializes and starts an HTTP/2 server listener on a specified address and port.
 *
 * This function sets up a TCP server specifically configured to handle the
 * HTTP/2 protocol. It leverages Quicpro's core networking engine for efficient
 * socket management and concurrency, allowing a single PHP application to
 * serve a high number of parallel HTTP/2 requests and streams.
 *
 * TLS is practically mandatory for browser-based HTTP/2, so the provided
 * `Quicpro\Config` object should be properly configured with certificates.
 * The server will handle the ALPN negotiation to select the 'h2' protocol.
 *
 * @param host_str The host address or IP on which the server should listen (e.g., "0.0.0.0").
 * @param host_len The length of the `host_str`.
 * @param port The port number for the listener (e.g., 443).
 * @param config_resource A PHP resource representing a `Quicpro\Config` object. This must
 * contain valid TLS configuration (`cert_file`, `key_file`) and should
 * include 'h2' in its `alpn` list.
 * @param request_handler_callable A unified PHP callable that will be invoked for each
 * incoming HTTP/2 request. It receives the request details and is responsible
 * for generating a response.
 * @return TRUE on successful server initialization.
 * FALSE on failure, throwing a `Quicpro\Exception\ServerException` or a
 * more specific exception detailing the error (e.g., TLS or configuration issue).
 */
PHP_FUNCTION(quicpro_http2_server_listen);

#endif // QUICPRO_SERVER_HTTP2_H
