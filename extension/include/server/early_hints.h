/* extension/include/server/early_hints.h */

#ifndef QUICPRO_SERVER_EARLY_HINTS_H
#define QUICPRO_SERVER_EARLY_HINTS_H

#include <php.h>

/**
 * @file extension/include/server/early_hints.h
 * @brief Public API declarations for sending HTTP 103 Early Hints.
 *
 * This header defines the interface for a performance-enhancing feature that
 * allows the server to send preliminary HTTP headers to the client before the
 * final response is ready. This is primarily used to send `Link` headers,
 * enabling the browser to start preloading critical sub-resources (like CSS,
 * JavaScript, or fonts) while the server is still processing a request
 * (e.g., waiting for a database query).
 */

/**
 * @brief Sends a 103 Early Hints response to the client.
 *
 * This function allows the server-side PHP application to send one or more
 * informational 103 responses on a given request stream. This is a non-blocking
 * operation that queues the hints for immediate transmission to the client.
 * It can be called multiple times before the final response (e.g., 200 OK)
 * is sent.
 *
 * @param session_resource The resource of the session handling the request.
 * @param stream_id The ID of the stream on which the request is being processed.
 * @param hints_array A PHP array of headers to be sent. This should primarily
 * contain `Link` headers as per the RFC 8297 specification.
 * Example: `[['Link', '</style.css>; rel=preload; as=style'], ['Link', '</script.js>; rel=preload; as=script']]`
 * @return TRUE on success, indicating the hints were successfully queued for sending.
 * FALSE on failure, for instance if the stream is already closed or if a final
 * response has already been sent. Throws a `Quicpro\Exception\StreamException`
 * for critical errors.
 */
PHP_FUNCTION(quicpro_server_send_early_hints);

#endif // QUICPRO_SERVER_EARLY_HINTS_H
