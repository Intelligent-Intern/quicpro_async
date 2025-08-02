#ifndef QUICPRO_CLIENT_INDEX_H
#define QUICPRO_CLIENT_INDEX_H

#include <php.h> // Essential for PHP_FUNCTION macro and fundamental PHP types.

/**
 * @file extension/include/client/index.h
 * @brief Centralized Client API for Quicpro.
 *
 * This header defines the primary entry point for all outbound HTTP requests
 * initiated from PHP userland through the Quicpro extension. It provides a
 * high-level, protocol-agnostic interface that intelligently selects and
 * manages the underlying transport and application protocols (HTTP/1.1, HTTP/2,
 * HTTP/3 over QUIC).
 *
 * The design incorporates advanced features like "Happy Eyeballs" for
 * optimized connection establishment across different IP families and
 * protocol versions, ensuring the lowest possible latency and highest
 * reliability under varying network conditions. It aims to be the definitive
 * and "perfect" client for all scenarios, from low-latency IoT communications
 * to high-throughput web service interactions.
 *
 * All request parameters, headers, bodies, and extensive options are passed
 * through to the chosen specialized client implementation (e.g., HTTP/1.1,
 * HTTP/2 via libcurl, or HTTP/3 via native quiche integration), ensuring
 * consistent behavior and comprehensive control regardless of the final
 * protocol decided upon.
 */

/**
 * @brief Sends an HTTP request with automatic protocol and IP family negotiation.
 *
 * This function serves as the intelligent, top-level client request dispatcher.
 * It automatically attempts to establish a connection and send an HTTP request
 * using the optimal protocol and IP family based on the provided configuration
 * (`Quicpro\Config`) or sensible defaults.
 *
 * The selection process follows a "Happy Eyeballs" principle, attempting preferred
 * protocols and IP families in parallel or with controlled fallbacks to minimize
 * perceived latency. For example, it might prioritize HTTP/3 over QUIC for speed,
 * falling back to HTTP/2 over TCP, and then to HTTP/1.1 if necessary.
 * IP family preference (IPv6 vs. IPv4) is also part of this negotiation.
 *
 * Explicit protocol and IP family preferences can be specified in the `options_array`
 * to override the automatic behavior, allowing for highly specialized use cases
 * (e.g., forcing IPv4 only for IoT devices).
 *
 * @param url_str The target URL for the HTTP request (e.g., "https://api.example.com").
 * This parameter is mandatory.
 * @param url_len The length of the `url_str`.
 * @param method_str The HTTP method (e.g., "GET", "POST", "PUT", "DELETE").
 * Defaults to "GET" if not provided.
 * @param method_len The length of the `method_str`.
 * @param headers_array An optional associative PHP array of request headers.
 * Example: `['Accept' => 'application/json']`.
 * @param body_zval The optional request body. This can be:
 * - A PHP string for standard request bodies.
 * - A PHP array or object, which will be encoded into IIBIN or JSON.
 * - A PHP stream resource, enabling zero-copy streaming for uploads.
 * @param options_array An optional associative PHP array for advanced configuration.
 * This array can specify protocol preferences (`preferred_protocol`),
 * IP family preferences (`preferred_ip_family`), Happy Eyeballs timeouts,
 * as well as all other generic `libcurl` or `quiche`-specific options
 * relevant to the chosen protocol. Key options for protocol selection:
 * - `preferred_protocol` (string): "auto" (default, Happy Eyeballs), "http1.1", "http2.0", "http3.0".
 * - `preferred_ip_family` (string): "auto" (default, Happy Eyeballs), "ipv4", "ipv6".
 * - `happy_eyeballs_quic_timeout_ms` (int): Timeout for initial QUIC attempt.
 * - `happy_eyeballs_tcp_timeout_ms` (int): Timeout for initial TCP attempt if parallel.
 * - `connection_config` (resource): An optional `Quicpro\Config` resource
 * for this specific connection, overriding global INI settings.
 * @return A PHP array on success, containing:
 * - 'status' (int): The HTTP response status code.
 * - 'body' (string|array|object|null): The response body, potentially
 * IIBIN-decoded or null if streamed.
 * - 'headers' (array): An associative array of normalized HTTP response headers.
 * Returns FALSE on failure, automatically throwing a specific `Quicpro\Exception`
 * subclass (e.g., `HttpClientException`, `QuicException`, `TlsException`).
 */
PHP_FUNCTION(quicpro_client_send_request);

#endif // QUICPRO_CLIENT_INDEX_H