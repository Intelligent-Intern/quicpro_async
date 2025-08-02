#ifndef QUICPRO_CLIENT_HTTP3_H
#define QUICPRO_CLIENT_HTTP3_H

#include <php.h>

/**
 * @file extension/include/client/http3.h
 * @brief Public API declarations for the Quicpro HTTP/3 Client.
 *
 * This header defines the PHP-callable function that facilitates making
 * HTTP/3 requests using Quicpro's native QUIC+HTTP/3 stack, built upon
 * the `quiche` library. It serves as the primary interface for users who
 * explicitly wish to utilize the HTTP/3 protocol for their outbound
 * client-side communications, leveraging its advanced features such as
 * 0-RTT connection establishment, improved multiplexing without head-of-line
 * blocking at the transport layer, and superior loss recovery.
 *
 * The `quicpro_http3_request_send` function is designed to be a comprehensive
 * endpoint for dispatching HTTP/3 requests, offering precise control over
 * request parameters and behavior, while ensuring the highest possible
 * performance and reliability through direct integration with `quiche`.
 */

/**
 * @brief Sends an HTTP/3 request to a specified URL over QUIC.
 *
 * This function allows PHP userland applications to perform HTTP/3 requests
 * using Quicpro's highly optimized, native QUIC transport layer. It directly
 * interacts with the `quiche` library for establishing QUIC connections and
 * managing HTTP/3 streams. This provides the lowest possible latency and
 * highest throughput for HTTP requests, especially in challenging network
 * conditions.
 *
 * Unlike `quicpro_http_request_send` (which uses libcurl for TCP-based HTTP),
 * this function utilizes Quicpro's internal QUIC session management.
 * The HTTP/3 protocol features such as HPACK header compression and stream
 * prioritization are handled by the `quiche_h3_conn` layer.
 *
 * @param url_str The target URL for the HTTP/3 request (e.g., "https://example.com/fast-api").
 * HTTP/3 inherently operates over HTTPS. This parameter is mandatory.
 * @param url_len The length of the `url_str`.
 * @param method_str The HTTP method (e.g., "GET", "POST", "PUT", "DELETE").
 * Defaults to "GET" if not provided.
 * @param method_len The length of the `method_str`.
 * @param headers_array An optional associative PHP array of request headers.
 * Example: `['Accept' => 'application/json', 'User-Agent' => 'QuicproClient/1.0']`.
 * Headers are processed and compressed according to HTTP/3's QPACK specification.
 * @param body_zval The optional request body. This can be:
 * - A PHP string for standard request bodies (e.g., JSON, XML).
 * - A PHP array or object, which will be encoded into IIBIN format
 * if the `request_iibin_schema` option is set, or into JSON otherwise.
 * - A PHP stream resource, enabling zero-copy streaming of large
 * request bodies directly from the stream, highly efficient for
 * large uploads over HTTP/3.
 * @param options_array An optional associative PHP array for advanced configuration.
 * This array can contain a multitude of key-value pairs that
 * directly influence the underlying QUIC session and HTTP/3 stream behavior.
 * Key options would include:
 * - `session_resource` (resource): An *existing* `Quicpro\Session` resource
 * to reuse a persistent QUIC connection. If not provided, a new session
 * will be attempted.
 * - `timeout_ms` (int): Total request timeout in milliseconds.
 * - `connect_timeout_ms` (int): Connection establishment timeout in milliseconds.
 * - `request_iibin_schema` (string): IIBIN schema name for encoding the request body.
 * - `response_iibin_schema` (string): IIBIN schema name for decoding the response body.
 * - `output_stream` (resource): A writable PHP stream for direct
 * response body streaming (zero-copy download).
 * - `tls_verify_peer` (bool): Whether to verify the server's TLS certificate.
 * - `tls_ca_file` (string): Path to a custom CA certificate file.
 * - `tls_client_cert_file` (string): Path to client certificate for mTLS.
 * - `tls_client_key_file` (string): Path to client private key for mTLS.
 * - `initial_max_data` (int): QUIC connection flow control limit.
 * - `initial_max_stream_data` (int): QUIC stream flow control limit.
 * - `happy_eyeballs_ip_family_timeout_ms` (int): Timeout for parallel IP family connection attempts.
 * - `happy_eyeballs_protocol_fallback_timeout_ms` (int): Timeout before falling back to TCP if QUIC fails.
 * @return A PHP array on success, containing:
 * - 'status' (int): The HTTP response status code (e.g., 200, 404).
 * - 'body' (string|array|object|null): The complete HTTP response body. If
 * `output_stream` was used, this is `null`. If `response_iibin_schema` was
 * used, this is the decoded IIBIN data (array/object).
 * - 'headers' (array): An associative array of normalized HTTP response headers.
 * Returns FALSE on failure, automatically throwing a `Quicpro\Exception\QuicException`
 * (or a more specific exception like `Quicpro\Exception\TlsException`)
 * with a detailed error message from the underlying QUIC/HTTP/3 stack.
 */
PHP_FUNCTION(quicpro_http3_request_send);

#endif // QUICPRO_CLIENT_HTTP3_H