#ifndef QUICPRO_CLIENT_HTTP2_H
#define QUICPRO_CLIENT_HTTP2_H

#include <php.h>

/**
 * @file extension/include/client/http2.h
 * @brief Public API declarations for the Quicpro HTTP/2 Client.
 *
 * This header defines the PHP-callable function that enables making
 * HTTP/2 requests. It serves as the primary interface for users who
 * explicitly wish to utilize the HTTP/2 protocol for their outbound
 * client-side communications. The design emphasizes a clear and efficient
 * API that abstracts the complexities of the HTTP/2 protocol, including
 * stream multiplexing and connection management, leveraging the libcurl library.
 *
 * The `quicpro_http2_request_send` function is crafted to be a comprehensive
 * endpoint for dispatching HTTP/2 requests, offering precise control over
 * request parameters and behavior, while ensuring high performance and reliability
 * through its libcurl backend, which inherently handles HTTP/2 features like
 * multiplexing over a single TCP connection.
 */

/**
 * @brief Sends an HTTP/2 request to a specified URL.
 *
 * This function allows PHP userland applications to perform HTTP/2 requests
 * with fine-grained control over various aspects of the request and response.
 * It acts as a specialized wrapper around the more generic `quicpro_http_request_send`
 * (which internally uses libcurl), explicitly setting the HTTP version to 2.0
 * and enabling HTTP/2-specific optimizations like connection multiplexing.
 *
 * The parameters are designed to encompass typical HTTP request components,
 * providing flexibility for a wide array of use cases from simple GET requests
 * to complex POST operations with custom headers and large bodies.
 * HTTP/2 benefits such as efficient header compression (HPACK) and stream
 * prioritization are managed by libcurl internally.
 *
 * @param url_str The target URL for the HTTP/2 request (e.g., "https://example.com/api").
 * This parameter is mandatory. HTTP/2 typically requires HTTPS.
 * @param url_len The length of the `url_str`.
 * @param method_str The HTTP method (e.g., "GET", "POST", "PUT", "DELETE").
 * Defaults to "GET" if not provided.
 * @param method_len The length of the `method_str`.
 * @param headers_array An optional associative PHP array of request headers.
 * Example: `['Content-Type' => 'application/json', 'X-Custom-Header' => 'value']`.
 * Headers are added to the request as key-value pairs and are subject to HTTP/2's
 * HPACK compression by libcurl.
 * @param body_zval The optional request body. This can be:
 * - A PHP string for standard request bodies (e.g., JSON, XML).
 * - A PHP array or object, which will be encoded into IIBIN format
 * if the `request_iibin_schema` option is set, or into JSON otherwise.
 * - A PHP stream resource, enabling zero-copy streaming of large
 * request bodies directly from the stream to the network. This is
 * highly efficient for large uploads over HTTP/2.
 * @param options_array An optional associative PHP array for advanced configuration.
 * This array can contain a multitude of key-value pairs that
 * directly map to underlying libcurl options, providing granular
 * control over timeouts, redirects, SSL/TLS settings, proxy usage,
 * DNS resolution, network interface binding, response body handling
 * (e.g., streaming to file), and IIBIN schema specifications.
 * HTTP/2 specific options might include:
 * - `stream_weight` (int): (If exposed by libcurl) HTTP/2 stream priority weight (1-256).
 * - `pipewait` (bool): (Implicitly handled by `http_version` "2.0") Wait for existing
 * multiplexed connections in the connection pool.
 * (This list is illustrative; the full range of options would be documented
 * in the `quicpro_http_request_send` function's documentation.)
 * @return A PHP array on success, containing:
 * - 'status' (int): The HTTP response status code (e.g., 200, 404).
 * - 'body' (string|array|object|null): The complete HTTP response body. If
 * `output_stream` was used, this is `null`. If `response_iibin_schema` was
 * used, this is the decoded IIBIN data (array/object).
 * - 'headers' (array): An associative array of normalized HTTP response headers.
 * Returns FALSE on failure, automatically throwing a `Quicpro\Exception\HttpClientException`
 * with a detailed error message.
 */
PHP_FUNCTION(quicpro_http2_request_send);

#endif // QUICPRO_CLIENT_HTTP2_H