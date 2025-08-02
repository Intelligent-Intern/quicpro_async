#ifndef QUICPRO_CLIENT_EARLY_HINTS_H
#define QUICPRO_CLIENT_EARLY_HINTS_H

#include <php.h> // Essential for PHP_FUNCTION macro and fundamental PHP types.

/**
 * @file extension/include/client/early_hints.h
 * @brief Client-Side HTTP Early Hints (103) Processing for Quicpro.
 *
 * This header defines the public API for client-side handling of HTTP 103 Early Hints.
 * Early Hints are a performance optimization where a server sends a preliminary
 * HTTP response containing `Link` headers (e.g., `rel=preload`, `rel=preconnect`)
 * before the final (e.g., 200 OK) response is ready. This allows the client to
 * proactively initiate network connections or download critical resources,
 * significantly improving perceived page load times and overall application responsiveness.
 *
 * This module provides functions to integrate the reception and processing of
 * these early hints into the client's request flow, enabling sophisticated
 * preloading and preconnecting strategies.
 */

/**
 * @brief Processes received HTTP 103 Early Hints for a given request context.
 *
 * This function is intended to be called by the core HTTP client (e.g.,
 * `quicpro_http3_request_send` or `quicpro_http_request_send`) when an
 * HTTP 103 (Early Hints) response is received. It parses the `Link`
 * headers contained within the Early Hint response and exposes them
 * to PHP userland, typically through a callback or by updating a
 * request-specific context.
 *
 * The PHP application can then interpret these hints and decide
 * on appropriate proactive actions, such as initiating parallel
 * requests for linked resources (e.g., video segments, CSS, JavaScript).
 * This enables true parallelization of resource fetching even while the
 * main request is still being processed by the server.
 *
 * @param request_context_resource A PHP resource representing the ongoing
 * HTTP request (e.g., the `Quicpro\HttpClientRequest` object, or a session resource).
 * This context is used to associate the hints with a specific ongoing operation. Mandatory.
 * @param headers_array A PHP array containing the headers received in the
 * 103 Early Hint response. This array is expected to be in the same format
 * as the `headers` element returned by `quicpro_http_request_send`. Mandatory.
 * @return TRUE if the Early Hints were successfully processed (e.g., parsed,
 * and associated with the request context). Returns FALSE on parsing errors
 * or invalid input. Throws a `Quicpro\Exception\ClientException` for relevant errors.
 */
PHP_FUNCTION(quicpro_client_early_hints_process);

/**
 * @brief Retrieves any pending Early Hints for a given request.
 *
 * This function allows the PHP application to query for Early Hints that
 * might have been received asynchronously for an ongoing HTTP request.
 * It is particularly useful in asynchronous or Fiber-based environments
 * where hints might arrive before the main response.
 *
 * @param request_context_resource A PHP resource representing the ongoing HTTP request. Mandatory.
 * @return A PHP array containing all parsed `Link` headers received as Early Hints
 * for this request so far. Each element in the array might represent a single
 * hinted resource, parsed into its components (URL, rel, as, etc.).
 * Returns an empty array if no Early Hints have been received or processed yet.
 * Returns FALSE on error.
 */
PHP_FUNCTION(quicpro_client_early_hints_get_pending);

#endif // QUICPRO_CLIENT_EARLY_HINTS_H