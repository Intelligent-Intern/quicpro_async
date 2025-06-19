/*
 * include/websocket.h – Public Interface for WebSocket over QUIC (Quicpro\WebSocket)
 * ==================================================================================
 *
 * This header file declares the function prototypes for the WebSocket API
 * functionalities provided by the php-quicpro_async extension. These functions
 * allow PHP userland to establish and manage WebSocket connections over QUIC/HTTP/3.
 *
 * Responsibilities:
 * - Declaring all PHP_FUNCTIONs related to WebSocket operations.
 *
 * This file is included by websocket.c for implementation and by the main
 * extension header (php_quicpro.h) to make these functions available for registration.
 * Argument information (arginfo) for these functions is located in php_quicpro_arginfo.h.
 */

#ifndef QUICPRO_WEBSOCKET_H
#define QUICPRO_WEBSOCKET_H

#include <php.h> // Required for the PHP_FUNCTION macro.

/*
 * PHP_FUNCTION(quicpro_ws_connect);
 * ---------------------------------
 * Establishes a direct WebSocket connection over QUIC/HTTP/3 to a remote endpoint.
 *
 * Userland Signature (from php_quicpro_arginfo.h):
 * resource quicpro_ws_connect(string $host, int $port, string $path [, array $headers = []])
 */
PHP_FUNCTION(quicpro_ws_connect);

/*
 * PHP_FUNCTION(quicpro_ws_close);
 * -------------------------------
 * Closes an active WebSocket connection.
 *
 * Userland Signature (from php_quicpro_arginfo.h):
 * bool quicpro_ws_close(resource $ws_connection)
 */
PHP_FUNCTION(quicpro_ws_close);

/*
 * PHP_FUNCTION(quicpro_ws_send);
 * -----------------------------
 * Sends data (text or binary frame) over an active WebSocket connection.
 *
 * Userland Signature (from php_quicpro_arginfo.h):
 * bool quicpro_ws_send(resource $ws_connection, string $data [, bool $is_binary = false])
 * (Note: The exact signature for $is_binary might vary based on final arginfo)
 */
PHP_FUNCTION(quicpro_ws_send);

/*
 * PHP_FUNCTION(quicpro_ws_receive);
 * --------------------------------
 * Receives data (a full message or a frame) from an active WebSocket connection.
 * This might be a blocking or non-blocking call depending on implementation.
 *
 * Userland Signature (from php_quicpro_arginfo.h):
 * string|false|null quicpro_ws_receive(resource $ws_connection [, int $timeout_ms = -1])
 */
PHP_FUNCTION(quicpro_ws_receive);

/*
 * PHP_FUNCTION(quicpro_ws_get_status);
 * -----------------------------------
 * Retrieves the current status or state of the WebSocket connection.
 *
 * Userland Signature (from php_quicpro_arginfo.h):
 * int|array quicpro_ws_get_status(resource $ws_connection)
 * (Return type might be an int (enum/constant) or an array with detailed status)
 */
PHP_FUNCTION(quicpro_ws_get_status);

/*
 * PHP_FUNCTION(quicpro_ws_get_last_error);
 * ---------------------------------------
 * Gets the last error message specific to WebSocket operations for the current scope.
 *
 * Userland Signature (from php_quicpro_arginfo.h):
 * string quicpro_ws_get_last_error(void)
 */
PHP_FUNCTION(quicpro_ws_get_last_error);

/*
 * PHP_FUNCTION(quicpro_ws_upgrade);
 * --------------------------------
 * Upgrades an existing QUIC/HTTP/3 stream to a WebSocket connection (RFC 9220).
 *
 * Userland Signature (from php_quicpro_arginfo.h):
 * resource|false quicpro_ws_upgrade(resource $session, string $path [, array $headers = []])
 */
PHP_FUNCTION(quicpro_ws_upgrade);

#endif /* QUICPRO_WEBSOCKET_H */