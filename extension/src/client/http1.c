#ifndef QUICPRO_CLIENT_HTTP1_H
#define QUICPRO_CLIENT_HTTP1_H

#include <php.h>

/**
 * @file extension/include/client/http1.h
 * @brief Public API declarations for the Quicpro HTTP/1.1 Client.
 *
 * This header defines the PHP-callable function that facilitates making
 * HTTP/1.1 requests. It serves as the primary interface for users who
 * explicitly wish to utilize the HTTP/1.1 protocol for their outbound
 * client-side communications. The design prioritizes a straightforward
 * API that encapsulates the underlying complexities of the HTTP/1.1
 * protocol and its interaction with the libcurl library.
 *
 * The `quicpro_http1_request_send` function is designed to be a comprehensive
 * endpoint for sending HTTP/1.1 requests, offering full control over
 * request parameters and behavior, while ensuring robustness and performance
 * through its libcurl backend.
 */

/**
 * @brief Sends an HTTP/1.1 request to a specified URL.
 *
 * This function allows PHP userland applications to perform HTTP/1.1 requests
 * with fine-grained control over various aspects of the request and response.
 * It acts as a specialized wrapper around the more generic `quicpro_http_request_send`
 * (which internally uses libcurl), explicitly setting the HTTP version to 1.1
 * and applying other optimizations or behaviors relevant to HTTP/1.1.
 *
 * The parameters mirror common HTTP request components, ensuring flexibility
 * for a wide range of use cases from simple GET requests to complex POST
 * operations with custom headers and bodies.
 *
 * @param url_str The target URL for the HTTP/1.1 request (e.g., "http://example.com/path").
 * This parameter is mandatory.
 * @param url_len The length of the `url_str`.
 * @param method_str The HTTP method (e.g., "GET", "POST", "PUT", "DELETE").
 * Defaults to "GET" if not provided.
 * @param method_len The length of the `method_str`.
 * @param headers_array An optional associative PHP array of request headers.
 * Example: `['Content-Type' => 'application/json', 'Accept' => 'text/html']`.
 * Headers are added to the request as key-value pairs.
 * @param body_zval The optional request body. This can be:
 * - A PHP string for standard request bodies (e.g., JSON, XML).
 * - A PHP array or object, which will be encoded into IIBIN format
 * if the `request_iibin_schema` option is set, or into JSON otherwise.
 * - A PHP stream resource, enabling zero-copy streaming of large
 * request bodies directly from the stream to the network.
 * @param options_array An optional associative PHP array for advanced configuration.
 * This array can contain a multitude of key-value pairs that
 * directly map to underlying libcurl options, providing granular
 * control over timeouts, redirects, SSL/TLS settings, proxy usage,
 * DNS resolution, network interface binding, response body handling
 * (e.g., streaming to file), and IIBIN schema specifications.
 * Key options would include:
 * - `timeout_ms` (int): Total request timeout in milliseconds.
 * - `connect_timeout_ms` (int): Connection timeout in milliseconds.
 * - `follow_redirects` (bool): Whether to automatically follow HTTP redirects.
 * - `output_stream` (resource): A writable PHP stream for direct
 * response body streaming (zero-copy download).
 * - `request_iibin_schema` (string): IIBIN schema name for encoding the request body.
 * - `response_iibin_schema` (string): IIBIN schema name for decoding the response body.
 * - `verify_peer` (bool): Whether to verify the peer's SSL certificate.
 * - `ca_info` (string): Path to a custom CA certificate file.
 * - `proxy` (string): Proxy URL.
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
PHP_FUNCTION(quicpro_http1_request_send);

#endif // QUICPRO_CLIENT_HTTP1_H