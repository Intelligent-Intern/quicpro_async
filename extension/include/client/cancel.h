#ifndef QUICPRO_CLIENT_CANCEL_H
#define QUICPRO_CLIENT_CANCEL_H

#include <php.h> // Essential for PHP_FUNCTION macro and fundamental PHP types.

/**
 * @file extension/include/client/cancel.h
 * @brief Client-Side Stream Cancellation for Quicpro.
 *
 * This header defines the public API for managing the cancellation or
 * graceful shutdown of individual HTTP streams from the client side.
 * In modern HTTP protocols (HTTP/2 and HTTP/3), streams are independent
 * logical channels multiplexed over a single connection. The ability to
 * cancel an unwanted stream is crucial for efficient resource management,
 * reducing unnecessary bandwidth consumption, and preventing server-side
 * processing of unneeded data.
 *
 * This module provides a mechanism for PHP userland to explicitly signal
 * to the server that a particular stream should stop sending data. This is
 * especially relevant for scenarios like partial content loading, video
 * streaming where users navigate away, or aborted downloads.
 *
 * It consolidates stream cancellation functionalities, previously part of
 * a more generic `cancel.h`, specifically for client-initiated actions.
 */

/**
 * @brief Cancels an active HTTP stream on the client side.
 *
 * This function sends a signal to the server to stop sending data on a
 * specific HTTP stream. This is typically achieved by sending a
 * `STOP_SENDING` frame in HTTP/3 or by explicitly closing the receive
 * side of a stream in HTTP/2/1.1 context. The client indicates that
 * it no longer requires data for this stream.
 *
 * The `how` parameter provides granular control over which direction
 * of the stream should be affected (e.g., stop receiving, stop sending, or both).
 *
 * @param stream_id The unique identifier of the HTTP stream to cancel.
 * This ID is typically returned when the request for that stream was initiated. Mandatory.
 * @param how_str A string indicating how the stream should be shut down:
 * - "read": Stop receiving data on this stream.
 * - "write": Stop sending data on this stream (client closes its sending side).
 * - "both": Stop both receiving and sending data (full stream close).
 * Defaults to "both" if not provided.
 * @param how_len The length of `how_str`.
 * @return TRUE on successful initiation of the cancellation process.
 * Returns FALSE on failure (e.g., invalid stream ID, stream already closed).
 * Throws a `Quicpro\Exception\StreamException` (or a more specific exception
 * like `Quicpro\Exception\QuicException` for underlying QUIC errors).
 */
PHP_FUNCTION(quicpro_client_stream_cancel);

#endif // QUICPRO_CLIENT_CANCEL_H