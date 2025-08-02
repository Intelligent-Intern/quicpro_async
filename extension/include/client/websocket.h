#ifndef QUICPRO_CLIENT_WEBSOCKET_H
#define QUICPRO_CLIENT_WEBSOCKET_H

#include <php.h> // Essential for PHP_FUNCTION macro and fundamental PHP types.

/**
 * @file extension/include/client/websocket.h
 * @brief Client-Side WebSocket Management for Quicpro.
 *
 * This header defines the public API for managing WebSocket connections
 * specifically from the client perspective within the Quicpro extension.
 * It provides a high-performance interface for real-time, bi-directional
 * communication, supporting both WebSocket over HTTP/3 (WebTransport/QUIC-based)
 * and traditional WebSocket over TCP/TLS (HTTP/1.1 or HTTP/2 upgrade).
 *
 * This module allows clients to initiate WebSocket handshakes, send and receive
 * various types of WebSocket frames (text, binary, ping, pong, close), and
 * manage the state of the WebSocket connection. It consolidates WebSocket-related
 * functionalities previously found in the legacy `websocket.h` file.
 */

/**
 * @brief Establishes a new WebSocket connection to a specified URL.
 *
 * This function initiates a WebSocket handshake with a server. It supports
 * upgrading an existing HTTP/3 (QUIC) session to a WebSocket (WebTransport-like behavior)
 * if the URL implies an HTTP/3 connection (e.g., via `https://` to a QUIC-enabled server).
 * Alternatively, it performs a standard HTTP/1.1 or HTTP/2 upgrade handshake
 * over TCP/TLS if the connection profile is TCP-based.
 *
 * This function returns a PHP resource representing the active WebSocket connection.
 *
 * @param url_str The full URL for the WebSocket connection (e.g., "wss://example.com/chat"). Mandatory.
 * @param url_len The length of `url_str`.
 * @param headers_array An optional associative PHP array of additional HTTP headers
 * to send during the WebSocket handshake (e.g., `Origin`, `Sec-WebSocket-Protocol`).
 * @param options_array An optional associative PHP array for advanced configuration,
 * including connection timeouts, TLS settings, proxy settings, and specifically:
 * - `connection_config` (resource): A `Quicpro\Config` resource overriding global settings.
 * - `preferred_protocol` (string): "http1.1", "http2.0", "http3.0", or "auto" for the underlying HTTP upgrade.
 * - `max_payload_size` (int): Maximum size of a single WebSocket message payload.
 * - `ping_interval_ms` (int): Interval for sending keep-alive PING frames.
 * - `handshake_timeout_ms` (int): Timeout for the WebSocket handshake completion.
 * @return A PHP resource representing the new `Quicpro\WebSocket` connection on success.
 * Returns FALSE on failure, automatically throwing a `Quicpro\Exception\WebSocketException`
 * (or a more specific exception like `Quicpro\Exception\HttpClientException` for HTTP errors,
 * or `Quicpro\Exception\TlsException` for TLS issues).
 */
PHP_FUNCTION(quicpro_client_websocket_connect);

/**
 * @brief Sends a message over an established WebSocket connection.
 *
 * This function allows sending text or binary data frames to the connected peer.
 * It handles the framing of WebSocket messages, including setting the correct
 * opcode, payload length, and masking.
 *
 * @param websocket_resource A PHP resource representing an active `Quicpro\WebSocket` connection. Mandatory.
 * @param data_str The raw data (text or binary) to send as the WebSocket message payload. Mandatory.
 * @param data_len The length of `data_str`.
 * @param is_binary A boolean flag. If TRUE, the message is sent as a binary frame (opcode 0x2).
 * If FALSE, it's sent as a text frame (opcode 0x1). Defaults to FALSE.
 * @return TRUE on successful sending of the WebSocket frame. Returns FALSE on failure
 * (e.g., connection closed, invalid state). Throws a `Quicpro\Exception\WebSocketException`.
 */
PHP_FUNCTION(quicpro_client_websocket_send);

/**
 * @brief Receives a message from an established WebSocket connection.
 *
 * This function attempts to read an incoming WebSocket message (text or binary)
 * from the connected peer. It handles the de-framing of received data, including
 * opcode interpretation, payload length decoding, and unmasking.
 *
 * @param websocket_resource A PHP resource representing an active `Quicpro\WebSocket` connection. Mandatory.
 * @param timeout_ms An optional timeout in milliseconds for waiting for data. If 0,
 * the function returns immediately if no data is available. If -1, it blocks indefinitely.
 * @return A PHP string containing the raw payload of the received message.
 * Returns an empty string if no message is available within the timeout.
 * Returns FALSE on connection close or error. Throws a `Quicpro\Exception\WebSocketException`.
 */
PHP_FUNCTION(quicpro_client_websocket_receive);

/**
 * @brief Sends a PING frame over an established WebSocket connection.
 *
 * PING frames are used as a keep-alive mechanism or to measure latency.
 * The connected peer is expected to respond with a PONG frame.
 *
 * @param websocket_resource A PHP resource representing an active `Quicpro\WebSocket` connection. Mandatory.
 * @param payload_str An optional payload for the PING frame (e.g., arbitrary data). Max 125 bytes.
 * @param payload_len The length of `payload_str`.
 * @return TRUE on successful sending of the PING frame. Returns FALSE on failure.
 * Throws a `Quicpro\Exception\WebSocketException`.
 */
PHP_FUNCTION(quicpro_client_websocket_ping);

/**
 * @brief Receives the current status of a WebSocket connection.
 *
 * This function provides information about the internal state of the WebSocket connection.
 * The status can indicate whether the connection is connecting, open, closing, or closed.
 *
 * @param websocket_resource A PHP resource representing a `Quicpro\WebSocket` connection. Mandatory.
 * @return A PHP long integer representing the connection status. (Defined constants will be provided for status values).
 */
PHP_FUNCTION(quicpro_client_websocket_get_status);

/**
 * @brief Retrieves the last error message associated with a WebSocket connection.
 *
 * This function provides a detailed error message if the WebSocket connection
 * has encountered a failure.
 *
 * @param websocket_resource A PHP resource representing a `Quicpro\WebSocket` connection. Mandatory.
 * @return A PHP string containing the last error message. Returns an empty string if no error occurred.
 */
PHP_FUNCTION(quicpro_client_websocket_get_last_error);

/**
 * @brief Closes an established WebSocket connection.
 *
 * This function initiates the graceful shutdown of a WebSocket connection by
 * sending a Close frame to the peer. It allows specifying a status code
 * and an optional reason for the closure.
 *
 * @param websocket_resource A PHP resource representing an active `Quicpro\WebSocket` connection. Mandatory.
 * @param status_code An optional 2-byte status code indicating the reason for closure (e.g., 1000 for Normal Closure).
 * @param reason_str An optional human-readable reason for closing. Max 123 bytes if combined with status_code.
 * @param reason_len The length of `reason_str`.
 * @return TRUE on successful initiation of closure. Returns FALSE on failure.
 * Throws a `Quicpro\Exception\WebSocketException`.
 */
PHP_FUNCTION(quicpro_client_websocket_close);


#endif // QUICPRO_CLIENT_WEBSOCKET_H