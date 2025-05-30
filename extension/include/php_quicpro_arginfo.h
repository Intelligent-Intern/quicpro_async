/* extension/include/php_quicpro_arginfo.h
 *
 * Argument information for all PHP functions provided by the php-quicpro_async
 * extension. These macros describe the expected parameter types, names,
 * and optionality to the Zend engine, enabling:
 *
 *   • Type checking at call time
 *   • Reflection of parameter names and types in userland
 *   • Generation of accurate stubs and documentation
 *
 * Each ZEND_BEGIN_ARG_INFO_EX block corresponds to one PHP_FUNCTION().
 * The entries inside list each parameter:
 *   – 0th argument: pass‑by‑value or pass‑by‑reference (0 = by value)
 *   – Parameter name as seen in PHP code
 *   – Expected type (IS_STRING, IS_LONG, IS_ARRAY, etc.)
 *   – Whether the parameter may be NULL (1 = nullable, 0 = required)
 *   – Add descriptions and all new WebSocket and MCP functions!
 */

#ifndef PHP_QUICPRO_ARGINFO_H
#define PHP_QUICPRO_ARGINFO_H

/*------------------------------------------------------------------------*/
/* quicpro_connect(string $host, int $port, resource $config [, array $options]): resource */
/*------------------------------------------------------------------------*/
/*
 * Open a new QUIC session to the given host and port.
 *   $host:    DNS name or IP address of the server
 *   $port:    UDP port number (e.g. 443 for HTTPS)
 *   $config:  A configuration resource from quicpro_new_config()
 *   $options: Optional advanced connection controls (family, delay, iface)
 *
 * Returns a session resource on success, or throws on failure.
 */
ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_connect, 0, 0, 3)
    ZEND_ARG_TYPE_INFO(0, host,    IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, port,    IS_LONG,   0)
    ZEND_ARG_INFO(0, config)
    ZEND_ARG_TYPE_INFO(0, options, IS_ARRAY,  1)
ZEND_END_ARG_INFO()

/*------------------------------------------------------------------------*/
/* quicpro_close(resource|object $session): bool */
/*------------------------------------------------------------------------*/
/*
 * Gracefully close an existing QUIC session or a single stream or resource.
 *   $session: The resource returned by quicpro_connect() or Quicpro\Session object.
 *
 * Returns true on success, false if the resource/object was invalid.
 */
ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_close, 0, 0, 1)
    ZEND_ARG_INFO(0, session)
ZEND_END_ARG_INFO()

/*------------------------------------------------------------------------*/
/* quicpro_send_request(resource $session, string $path, array|null $headers, string|null $body): int */
/*------------------------------------------------------------------------*/
/*
 * Send an HTTP/3 request on the given session.
 *   $session:  The resource returned by quicpro_connect()
 *   $path:     The request target (e.g. "/index.html")
 *   $headers:  Optional associative array of additional headers
 *   $body:     Optional request body to send (for POST/PUT)
 *
 * Returns a numeric stream ID for the request, or throws on error.
 */
ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_send_request, 0, 0, 2)
    ZEND_ARG_INFO(0, session)
    ZEND_ARG_TYPE_INFO(0, path,    IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, headers, IS_ARRAY,  1)
    ZEND_ARG_TYPE_INFO(0, body,    IS_STRING, 1)
ZEND_END_ARG_INFO()

/*------------------------------------------------------------------------*/
/* quicpro_receive_response(resource $session, int $stream_id): array|null */
/*------------------------------------------------------------------------*/
/*
 * Receive the full HTTP/3 response for a given stream.
 *   $session:   The resource returned by quicpro_connect()
 *   $stream_id: The stream ID returned by quicpro_send_request()
 *
 * Returns the response as an array (headers, body), or NULL if not ready.
 */
ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_receive_response, 0, 0, 2)
    ZEND_ARG_INFO(0, session)
    ZEND_ARG_TYPE_INFO(0, stream_id, IS_LONG, 0)
ZEND_END_ARG_INFO()

/*------------------------------------------------------------------------*/
/* quicpro_poll(resource $session, int $timeout_ms): bool */
/*------------------------------------------------------------------------*/
/*
 * Perform one iteration of the event loop for the given session.
 *   $session:    The resource returned by quicpro_connect()
 *   $timeout_ms: Maximum time in milliseconds to block (negative = indefinite)
 *
 * Drains incoming packets, sends outgoing packets, and handles timeouts.
 * Returns true on success, false on invalid session.
 */
ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_poll, 0, 0, 2)
    ZEND_ARG_INFO(0, session)
    ZEND_ARG_TYPE_INFO(0, timeout_ms, IS_LONG, 0)
ZEND_END_ARG_INFO()

/*------------------------------------------------------------------------*/
/* quicpro_cancel_stream(resource $session, int $stream_id): bool */
/*------------------------------------------------------------------------*/
/*
 * Abruptly shutdown a specific HTTP/3 stream for reading/writing.
 *   $session:   The resource returned by quicpro_connect()
 *   $stream_id: The stream ID to cancel
 *
 * Returns true on success, false on error or invalid stream.
 */
ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_cancel_stream, 0, 0, 2)
    ZEND_ARG_INFO(0, session)
    ZEND_ARG_TYPE_INFO(0, stream_id, IS_LONG, 0)
ZEND_END_ARG_INFO()

/*------------------------------------------------------------------------*/
/* quicpro_export_session_ticket(resource $session): string */
/*------------------------------------------------------------------------*/
/*
 * Export the latest TLS session ticket from an active QUIC session.
 *   $session: The resource returned by quicpro_connect()
 *
 * Returns a binary string containing the ticket, or an empty string
 * if no ticket is available yet.
 */
ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_export_session_ticket, 0, 0, 1)
    ZEND_ARG_INFO(0, session)
ZEND_END_ARG_INFO()

/*------------------------------------------------------------------------*/
/* quicpro_import_session_ticket(resource $session, string $ticket): bool */
/*------------------------------------------------------------------------*/
/*
 * Import a previously exported TLS session ticket into a new session,
 * enabling 0‑RTT handshake resumption.
 *   $session: The resource returned by quicpro_connect()
 *   $ticket:  The binary ticket string returned by export_session_ticket()
 *
 * Returns true on success, false if ticket is invalid or rejected.
 */
ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_import_session_ticket, 0, 0, 2)
    ZEND_ARG_INFO(0, session)
    ZEND_ARG_TYPE_INFO(0, ticket, IS_STRING, 0)
ZEND_END_ARG_INFO()

/*------------------------------------------------------------------------*/
/* quicpro_set_ca_file(resource $config, string $ca_file): bool */
/*------------------------------------------------------------------------*/
/*
 * Set the file path for the CA bundle to use in all future QUIC configs.
 *   $config:  Config resource to modify (not frozen)
 *   $ca_file: Path to a PEM‑formatted CA certificate bundle
 *
 * Returns true on success.  Does not affect already‑open sessions.
 */
ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_set_ca_file, 0, 0, 2)
    ZEND_ARG_INFO(0, config)
    ZEND_ARG_TYPE_INFO(0, ca_file, IS_STRING, 0)
ZEND_END_ARG_INFO()

/*------------------------------------------------------------------------*/
/* quicpro_set_client_cert(resource $config, string $cert_pem, string $key_pem): bool */
/*------------------------------------------------------------------------*/
/*
 * Set the client certificate and private key files for mutual TLS.
 *   $config:   Config resource to modify (not frozen)
 *   $cert_pem: Path to the PEM‑encoded client certificate chain
 *   $key_pem:  Path to the PEM‑encoded private key
 *
 * Returns true on success.  Applies only to future new_config calls.
 */
ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_set_client_cert, 0, 0, 3)
    ZEND_ARG_INFO(0, config)
    ZEND_ARG_TYPE_INFO(0, cert_pem, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, key_pem,  IS_STRING, 0)
ZEND_END_ARG_INFO()

/*------------------------------------------------------------------------*/
/* quicpro_get_last_error(): string */
/*------------------------------------------------------------------------*/
/*
 * Retrieve the last extension‑level error message set by any function.
 *
 * Returns the error text, or an empty string if no error is pending.
 */
ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_get_last_error, 0, 0, 0)
ZEND_END_ARG_INFO()

/*------------------------------------------------------------------------*/
/* quicpro_get_stats(resource $session): array */
/*------------------------------------------------------------------------*/
/*
 * Return transport‑level statistics for the given session:
 *   pkt_rx, pkt_tx, lost, rtt_ns, cwnd, etc.
 *
 *   $session: The resource returned by quicpro_connect()
 *
 * Returns an associative array of metric names to integer values.
 */
ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_get_stats, 0, 0, 1)
    ZEND_ARG_INFO(0, session)
ZEND_END_ARG_INFO()

/*------------------------------------------------------------------------*/
/* quicpro_version(): string */
/*------------------------------------------------------------------------*/
/*
 * Return the version string of the loaded php-quicpro_async extension.
 * This corresponds to PHP_QUICPRO_VERSION from php_quicpro.h.
 */
ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_version, 0, 0, 0)
ZEND_END_ARG_INFO()

/*------------------------------------------------------------------------*/
/* ---------------------- WebSocket API ----------------------------------*/
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* quicpro_ws_connect(string $host, int $port, string $path, array $headers): resource */
/*
 * Open a direct WebSocket connection using QUIC/HTTP3 from PHP to a remote
 * WebSocket endpoint (usually not used in PHP, for completeness).
 */
ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_ws_connect, 0, 0, 3)
    ZEND_ARG_TYPE_INFO(0, host,    IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, port,    IS_LONG,   0)
    ZEND_ARG_TYPE_INFO(0, path,    IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, headers, IS_ARRAY,  1)
ZEND_END_ARG_INFO()

/*------------------------------------------------------------------------*/
/* quicpro_ws_upgrade(resource $session, string $path, array $headers): resource */
/*
 * Upgrade an open HTTP/3 session and stream to a WebSocket (RFC 9220).
 *   $session:  Existing session from quicpro_connect()
 *   $path:     The path to upgrade (e.g. "/chat")
 *   $headers:  Optional headers (for authentication, JWT, etc)
 *
 * Returns a resource representing the upgraded WebSocket stream.
 */
ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_ws_upgrade, 0, 0, 2)
    ZEND_ARG_INFO(0, session)
    ZEND_ARG_TYPE_INFO(0, path,    IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, headers, IS_ARRAY,  1)
ZEND_END_ARG_INFO()

/*------------------------------------------------------------------------*/
/* quicpro_ws_send(resource $ws, string $data): bool */
/*
 * Send data (text or binary) over an open WebSocket connection.
 *   $ws:   WebSocket resource from ws_upgrade()
 *   $data: String payload to send
 *
 * Returns true on success, false on failure.
 */
ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_ws_send, 0, 0, 2)
    ZEND_ARG_INFO(0, ws)
    ZEND_ARG_TYPE_INFO(0, data, IS_STRING, 0)
ZEND_END_ARG_INFO()

/*------------------------------------------------------------------------*/
/* quicpro_ws_receive(resource $ws, int $timeout_ms = -1): string|null */
/*
 * Receive a message from an open WebSocket connection, blocking up to timeout_ms.
 *   $ws:         WebSocket resource from ws_upgrade()
 *   $timeout_ms: Max ms to wait (-1 = infinite)
 *
 * Returns received message as string, or NULL if connection closed or timed out.
 */
ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_ws_receive, 0, 0, 1)
    ZEND_ARG_INFO(0, ws)
    ZEND_ARG_TYPE_INFO(0, timeout_ms, IS_LONG, 1)
ZEND_END_ARG_INFO()

/*------------------------------------------------------------------------*/
/* quicpro_ws_close(resource $ws): bool */
/*
 * Close an open WebSocket connection, freeing all associated resources.
 *   $ws: WebSocket resource from ws_upgrade()
 *
 * Returns true on success, false if already closed or invalid.
 */
ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_ws_close, 0, 0, 1)
    ZEND_ARG_INFO(0, ws)
ZEND_END_ARG_INFO()

/*------------------------------------------------------------------------*/
/* quicpro_ws_get_status(resource $ws): int */
/*
 * Retrieve the current status code (open, closed, error) of a WebSocket stream.
 *   $ws: WebSocket resource
 *
 * Returns an integer status code.
 */
ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_ws_get_status, 0, 0, 1)
    ZEND_ARG_INFO(0, ws)
ZEND_END_ARG_INFO()

/*------------------------------------------------------------------------*/
/* quicpro_ws_get_last_error(): string */
/*
 * Returns the last WebSocket-related error message.
 */
ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_ws_get_last_error, 0, 0, 0)
ZEND_END_ARG_INFO()

/*------------------------------------------------------------------------*/
/* -------------------- MCP (Model Context Protocol) API -----------------*/
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* quicpro_mcp_connect(string $host, int $port, array $options): resource */
/*
 * Connect to an MCP (Model Context Protocol) server.
 *   $host:    Server host
 *   $port:    Server port
 *   $options: Connection options (optional)
 *
 * Returns a resource handle to the MCP session.
 */
ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_mcp_connect, 0, 0, 2)
    ZEND_ARG_TYPE_INFO(0, host, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, port, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(0, options, IS_ARRAY, 1)
ZEND_END_ARG_INFO()

/*------------------------------------------------------------------------*/
/* quicpro_mcp_disconnect(resource $mcp): bool */
/*
 * Disconnect from an MCP session, closing all streams and freeing resources.
 *   $mcp: MCP resource
 *
 * Returns true on success, false if already disconnected.
 */
ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_mcp_disconnect, 0, 0, 1)
    ZEND_ARG_INFO(0, mcp)
ZEND_END_ARG_INFO()

/*------------------------------------------------------------------------*/
/* quicpro_mcp_list_tools(resource $mcp): array */
/*
 * List all available tools on the connected MCP server.
 *   $mcp: MCP resource
 *
 * Returns an array of tool names.
 */
ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_mcp_list_tools, 0, 0, 1)
    ZEND_ARG_INFO(0, mcp)
ZEND_END_ARG_INFO()

/*------------------------------------------------------------------------*/
/* quicpro_mcp_invoke_tool(resource $mcp, string $tool, array $args): mixed */
/*
 * Invoke a specific tool via the MCP protocol, passing arguments.
 *   $mcp:  MCP resource
 *   $tool: Name of the tool to invoke
 *   $args: Arguments for the tool as an associative array
 *
 * Returns the result from the tool, type depends on implementation.
 */
ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_mcp_invoke_tool, 0, 0, 3)
    ZEND_ARG_INFO(0, mcp)
    ZEND_ARG_TYPE_INFO(0, tool, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, args, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

/*------------------------------------------------------------------------*/
/* quicpro_mcp_list_resources(resource $mcp): array */
/*
 * List all resources managed by the MCP server.
 *   $mcp: MCP resource
 *
 * Returns an array of resource names/IDs.
 */
ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_mcp_list_resources, 0, 0, 1)
    ZEND_ARG_INFO(0, mcp)
ZEND_END_ARG_INFO()

/*------------------------------------------------------------------------*/
/* quicpro_mcp_get_resource(resource $mcp, string $resource): mixed */
/*
 * Retrieve a specific resource by name or ID from the MCP server.
 *   $mcp:      MCP resource
 *   $resource: Name or ID of the resource
 *
 * Returns the resource, type depends on backend.
 */
ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_mcp_get_resource, 0, 0, 2)
    ZEND_ARG_INFO(0, mcp)
    ZEND_ARG_TYPE_INFO(0, resource, IS_STRING, 0)
ZEND_END_ARG_INFO()

/*------------------------------------------------------------------------*/
/* quicpro_mcp_fetch_data(resource $mcp, string $key): mixed */
/*
 * Perform a generic data fetch from the MCP server by key or identifier.
 *   $mcp: MCP resource
 *   $key: Key or identifier to fetch
 *
 * Returns the fetched value/data.
 */
ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_mcp_fetch_data, 0, 0, 2)
    ZEND_ARG_INFO(0, mcp)
    ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
ZEND_END_ARG_INFO()

/*------------------------------------------------------------------------*/
/* quicpro_mcp_get_last_error(): string */
/*
 * Returns the last error message related to MCP operations.
 */
ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_mcp_get_last_error, 0, 0, 0)
ZEND_END_ARG_INFO()

#endif /* PHP_QUICPRO_ARGINFO_H */
