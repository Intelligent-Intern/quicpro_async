/*
 * include/http3.h – Public interface for HTTP/3 operations in php-quicpro_async
 * =============================================================================
 *
 * This header file declares the function prototypes for the core HTTP/3
 * request sending and response receiving functionalities provided by the
 * php-quicpro_async extension. These functions are exposed to PHP userland.
 *
 * It is included by http3.c for implementation and by the main extension
 * file (e.g., php_quicpro.c) for function registration.
 */

#ifndef QUICPRO_HTTP3_H
#define QUICPRO_HTTP3_H

#include <php.h> // For PHP_FUNCTION macro and zval types if needed in signatures, though not directly here.

/*
 * PHP_FUNCTION(quicpro_send_request);
 * -----------------------------------
 * Sends an HTTP/3 request over an established QUIC session.
 *
 * Userland Signature (from php_quicpro_arginfo.h):
 * quicpro_send_request(resource $session, string $method, string $path [, array $headers [, string $body ]]): int|false
 *
 * Parameters (in C):
 * - z_sess:    PHP resource for the quicpro_session_t.
 * - z_method:  zend_string* for the HTTP method (e.g., "GET", "POST").
 * - z_path:    zend_string* for the request path (e.g., "/index.html").
 * - z_headers: zval* (IS_ARRAY or IS_NULL) for additional HTTP headers.
 * - z_body:    zend_string* (or IS_NULL) for the request body.
 *
 * Returns:
 * - A 64-bit stream ID (as zend_long) on success.
 * - FALSE on failure (an exception is also typically thrown).
 */
PHP_FUNCTION(quicpro_send_request);

/*
 * PHP_FUNCTION(quicpro_receive_response);
 * ---------------------------------------
 * Receives an HTTP/3 response for a given stream ID on an established QUIC session.
 * This function polls for HTTP/3 events and processes them.
 *
 * Userland Signature (from php_quicpro_arginfo.h):
 * quicpro_receive_response(resource $session, int $stream_id): array|string|null|false
 *
 * Parameters (in C):
 * - z_sess:     PHP resource for the quicpro_session_t.
 * - stream_id:  zend_long representing the stream ID from which to receive.
 *
 * Returns:
 * - A string containing concatenated headers if a HEADERS event is processed.
 * - A string containing body data if a DATA event is processed.
 * - NULL if no event is ready yet (QUICHE_H3_POLL_AGAIN_LATER equivalent, if applicable after quiche_h3_conn_poll returns 0).
 * - FALSE on protocol error (an exception is also typically thrown).
 * - Note: The actual PHP return type might be more structured depending on how
 * you decide to present headers (e.g., array vs. single string) and body chunks.
 * The provided C code returns headers as a single string and data chunks as strings.
 */
PHP_FUNCTION(quicpro_receive_response);

#endif /* QUICPRO_HTTP3_H */