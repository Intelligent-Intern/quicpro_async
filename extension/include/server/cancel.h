/* extension/include/server/cancel.h */

#ifndef QUICPRO_SERVER_CANCEL_H
#define QUICPRO_SERVER_CANCEL_H

#include <php.h>

/**
 * @file extension/include/server/cancel.h
 * @brief Public API declarations for handling request cancellation.
 *
 * This header defines the interface for the server to react to client-side
 * request cancellations. In protocols like HTTP/2 and HTTP/3, a client can
 * signal that it is no longer interested in the response to a request
 * (e.g., by sending an RST_STREAM frame). This API provides a mechanism for
 * the C-core to notify the PHP application, allowing it to gracefully
 * terminate long-running operations like database queries or file generation,
 * thereby saving server resources.
 */

/**
 * @brief Registers a cancellation handler for a specific request stream.
 *
 * This function associates a PHP callable with a specific stream ID. If the
 * client cancels the request associated with this stream, the C-core will
 * invoke the provided callable. This allows the PHP application to perform
 * cleanup operations, such as terminating a database query or deleting
 * temporary files.
 *
 * @param session_resource The resource of the session containing the stream.
 * @param stream_id The ID of the stream for which to register the handler.
 * @param cancel_handler_callable A PHP callable that will be invoked upon
 * cancellation. The callable will receive the `stream_id` as its only
 * argument, allowing it to identify which request was cancelled.
 * @return TRUE on success, indicating the handler was registered.
 * FALSE on failure (e.g., invalid stream ID or session).
 */
PHP_FUNCTION(quicpro_server_on_cancel);

#endif // QUICPRO_SERVER_CANCEL_H
