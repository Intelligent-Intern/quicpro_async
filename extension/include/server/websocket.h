/* extension/include/server/websocket.h */

#ifndef QUICPRO_SERVER_WEBSOCKET_H
#define QUICPRO_SERVER_WEBSOCKET_H

#include <php.h>

/**
 * @file extension/include/server/websocket.h
 * @brief Public API declarations for the Quicpro WebSocket Server.
 *
 * This header defines the PHP interface for handling WebSocket connections,
 * which are established over an underlying HTTP/3 connection via the CONNECT
 * method (RFC 9220). These functions abstract the complexities of the
 * WebSocket protocol, including the initial handshake and message framing,
 * providing a simple and efficient way to build real-time web applications.
 */

/**
 * @brief Upgrades an existing HTTP/3 session to a WebSocket connection.
 *
 * This function is called on the server after receiving a `CONNECT` request
 * targeting a WebSocket endpoint. It validates the request headers and, if
 * successful, transitions the underlying QUIC stream into a transparent
 * WebSocket message channel.
 *
 * @param session_resource The resource of the existing `Quicpro\Session`.
 * @param stream_id The ID of the stream on which the `CONNECT` request was received.
 * @return A new, distinct WebSocket connection resource on success.
 * Returns NULL on failure, throwing a `Quicpro\Exception\WebSocketException`
 * if the upgrade handshake is invalid or fails.
 */
PHP_FUNCTION(quicpro_server_upgrade_to_websocket);

/**
 * @brief Sends a WebSocket message from the server to a client.
 *
 * This function encapsulates a message payload into one or more WebSocket
 * frames and queues it for sending over the connection's stream. It handles
 * the framing logic and supports sending both text (UTF-8) and binary messages.
 *
 * @param websocket_connection_resource The WebSocket connection resource returned
 * by `quicpro_server_upgrade_to_websocket`.
 * @param message_str A string containing the message payload.
 * @param message_len The length of the message payload.
 * @param is_binary A boolean flag indicating the message type:
 * - `TRUE` for a binary message (opcode 0x2).
 * - `FALSE` for a text message (opcode 0x1).
 * @return TRUE on success, indicating the message was queued.
 * FALSE on failure (e.g., if the connection is already closed).
 */
PHP_FUNCTION(quicpro_websocket_send);

#endif // QUICPRO_SERVER_WEBSOCKET_H
