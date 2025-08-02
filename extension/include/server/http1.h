#ifndef QUICPRO_SERVER_HTTP1_H
#define QUICPRO_SERVER_HTTP1_H

#include <php.h>

/**
 * @file extension/include/server/http1.h
 * @brief Public API declarations for the Quicpro HTTP/1.1 Server.
 *
 * This header defines the PHP-callable functions that facilitate running
 * an HTTP/1.1 server. It serves as the primary interface for users who
 * explicitly wish to set up a server capable of handling HTTP/1.1 requests
 * over TCP. The design prioritizes a straightforward API that encapsulates
 * the underlying complexities of TCP socket management, HTTP/1.1 protocol
 * parsing, and request routing to user-defined handlers.
 *
 * The `quicpro_http1_server_listen` function initiates the server process,
 * allowing it to accept incoming HTTP/1.1 connections. It is designed to be
 * a robust and performant solution for traditional web serving needs,
 * operating within the Quicpro ecosystem.
 */

/**
 * @brief Initializes and starts an HTTP/1.1 server listener on a specified address and port.
 *
 * This function sets up a TCP server that listens for incoming HTTP/1.1 connections.
 * It integrates with Quicpro's internal configuration system to apply various
 * network, security, and protocol-specific settings. The server operates
 * asynchronously, allowing for efficient handling of multiple concurrent connections.
 *
 * Incoming HTTP/1.1 requests will be parsed, and if configured, dispatched
 * to user-defined PHP callbacks or an internal routing mechanism.
 *
 * @param host_str The host address or IP on which the server should listen (e.g., "0.0.0.0" for all interfaces, "127.0.0.1" for loopback). This parameter is mandatory.
 * @param host_len The length of the `host_str`.
 * @param port The port number on which the server should listen (e.g., 80, 8080). This parameter is mandatory.
 * @param config_resource A PHP resource representing a `Quicpro\Config` object. This object holds
 * all the configured settings for the server, including TCP transport
 * options, TLS/SSL settings (if HTTPS is implied by port or config),
 * security policies, and logging parameters.
 * @param request_handler_callable A PHP callable (e.g., a function name or an array representing
 * an object method) that will be invoked for each incoming HTTP/1.1
 * request. This handler will receive parameters representing the
 * HTTP request (e.g., method, URL, headers, body) and is expected
 * to return an appropriate HTTP response. This parameter is mandatory.
 * @return TRUE on successful server initialization and start of listening.
 * FALSE on failure, automatically throwing a `Quicpro\Exception\ServerException`
 * (or a more specific exception like `Quicpro\Exception\TcpException`
 * or `Quicpro\Exception\ConfigException`) with a detailed error message.
 */
PHP_FUNCTION(quicpro_http1_server_listen);

#endif // QUICPRO_SERVER_HTTP1_H