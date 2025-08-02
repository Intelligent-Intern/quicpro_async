/* extension/include/server/index.h */

#ifndef QUICPRO_SERVER_INDEX_H
#define QUICPRO_SERVER_INDEX_H

#include <php.h>

/**
 * @file extension/include/server/index.h
 * @brief Public API for the unified, multi-protocol Quicpro Server.
 *
 * This header defines the master entry point for creating and running a
 * high-performance, multi-protocol server instance within the Quicpro
 * framework. It exposes a single, powerful function that abstracts away
 * the complexity of managing different transport and application protocols,
 * allowing for a truly unified server implementation.
 *
 * The core philosophy is to provide a "perfect server" solution that can
 * intelligently listen for HTTP/1.1, HTTP/2, and HTTP/3 traffic
 * simultaneously, routing all requests to a single, unified PHP handler.
 * This approach maximizes compatibility and performance while minimizing
 * configuration overhead.
 */

/**
 * @brief Initializes and starts a multi-protocol server listener.
 *
 * This function serves as the intelligent, top-level server dispatcher.
 * It sets up listening sockets for various protocols based on the provided
 * `config_resource`. It supports simultaneous listening for:
 * - HTTP/1.1 over TCP
 * - HTTP/2 over TCP (typically with TLS via ALPN)
 * - HTTP/3 over QUIC (UDP)
 *
 * The server is designed to operate on the same host and port for different
 * protocols where standards allow (e.g., HTTP/2 and HTTP/3 often coexist on
 * port 443, with HTTP/3 being offered via Alt-Svc headers).
 *
 * The `config_resource` is paramount, guiding the server's behavior across
 * all layers, from low-level network tuning to application-layer protocol specifics
 * and security policies. The `request_handler_callable` is a unified entry point
 * for all incoming requests, regardless of the protocol, allowing PHP userland
 * to implement a single application logic for all HTTP traffic.
 *
 * @param host_str The host address or IP on which the server should listen (e.g., "0.0.0.0" for all interfaces).
 * This parameter is mandatory.
 * @param host_len The length of the `host_str`.
 * @param port The port number on which the server should listen (e.g., 80, 443). This parameter is mandatory.
 * @param config_resource A PHP resource representing a `Quicpro\Config` object. This object
 * encapsulates all server-side configurations, including but not limited to:
 * - TCP transport settings (e.g., `tcp_enable`, `tcp_max_connections`).
 * - QUIC transport settings (e.g., `cc_algorithm`, `datagrams_enable`).
 * - TLS and Crypto settings (e.g., `cert_file`, `key_file`).
 * - Application protocol settings (e.g., `h3_server_push_enable`).
 * - Security and traffic management (e.g., `rate_limiter_enable`).
 * - Cluster and process management (e.g., `cluster_workers`).
 * The server's listening capabilities across HTTP/1.1, HTTP/2, and HTTP/3
 * will be implicitly determined by the configuration within this resource.
 * @param request_handler_callable A PHP callable (e.g., a static method, a function name)
 * that will be invoked for every incoming HTTP request, regardless of its
 * protocol (HTTP/1.1, HTTP/2, HTTP/3). This callable must be capable of
 * processing the request details and returning a valid HTTP response.
 * This parameter is mandatory.
 * @return TRUE on successful server initialization and commencement of listening
 * across all configured protocols.
 * FALSE on failure, automatically throwing a `Quicpro\Exception\ServerException`
 * (or a more specific exception like `Quicpro\Exception\TcpException`,
 * `Quicpro\Exception\QuicException`, `Quicpro\Exception\TlsException`,
 * or `Quicpro\Exception\ConfigException`) with a detailed error message.
 */
PHP_FUNCTION(quicpro_server_listen);

#endif // QUICPRO_SERVER_INDEX_H
