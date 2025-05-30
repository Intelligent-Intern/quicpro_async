/*
 * include/mcp.h – Public Interface for Model Context Protocol (MCP) Operations
 * ============================================================================
 *
 * This header file declares the function prototypes for the core MCP client
 * functionalities provided by the php-quicpro_async extension. These functions
 * enable PHP userland to establish MCP connections, send requests, handle
 * responses, and manage data streams over QUIC.
 *
 * The PipelineOrchestrator will use these underlying C functions (or their internal
 * counterparts) to execute pipeline steps that involve MCP agent communication.
 * PHP userland can also use these functions, typically via a `Quicpro\MCP` PHP class
 * wrapper, for direct MCP interactions.
 *
 * Argument information (arginfo) for these functions is in php_quicpro_arginfo.h.
 */

#ifndef QUICPRO_MCP_H
#define QUICPRO_MCP_H

#include <php.h> // Required for PHP_FUNCTION macro

/*
 * PHP_FUNCTION(quicpro_mcp_connect);
 * ----------------------------------
 * Establishes an MCP connection to a remote MCP agent/service over QUIC.
 * This function is the foundation for `new Quicpro\MCP(...)` in PHP.
 *
 * Userland Signature (conceptual, actual in php_quicpro_arginfo.h):
 * resource|false quicpro_mcp_connect(string $host, int $port [, array $options = []])
 *
 * $options may include:
 * - tls_enable (bool, default: true)
 * - tls_ca_file (string, default: system CA bundle path determined by OpenSSL)
 * - tls_verify_peer (bool, default: true)
 * - tls_sni_hostname (string, default: $host value)
 * - tls_client_cert_file (string, path to client PEM certificate file)
 * - tls_client_key_file (string, path to client PEM private key file)
 * - connect_timeout_ms (int, milliseconds, e.g., 5000)
 * - request_timeout_ms (int, milliseconds, e.g., 30000, default for requests on this connection)
 * - session_mode (int, mapping to PHP constants like):
 * - Quicpro\MCP::MODE_RELIABLE_STREAMS (0) : Default. All operations use reliable QUIC streams.
 * - Quicpro\MCP::MODE_PREFER_UNRELIABLE_DATAGRAMS (1) : Configures connection for QUIC datagrams (if supported by peer and specific MCP functions).
 * - Quicpro\MCP::MODE_MIXED_STREAM_DATAGRAM (2) : Configures connection to support both streams and datagrams.
 * - zero_copy_send (bool, default: false, attempt to use zero-copy for sending)
 * - zero_copy_receive (bool, default: false, attempt to use zero-copy for receiving)
 * - mmap_path_prefix (string, for memory-mapped I/O if zero_copy is enabled and uses it)
 * - cpu_affinity_core (int, e.g. 3, to pin connection's I/O processing to a specific core if possible)
 * - busy_poll_duration_us (int, e.g. 500, microseconds for busy-polling)
 * - io_uring_enable (bool, default: false, attempt to use io_uring on Linux)
 * - disable_congestion_control (bool, default: false, DANGER: only for controlled networks)
 * - max_idle_timeout_ms (int, e.g. 300000, QUIC connection idle timeout)
 * - initial_max_data (int, bytes, QUIC initial max data for connection-level flow control)
 * - initial_max_stream_data_bidi_local (int, bytes, QUIC initial max data for locally-initiated bidi streams)
 * - max_concurrent_streams (int, e.g. 100, hint for QUIC max concurrent bidirectional streams)
 * - log_level (int, mapping to PHP constants like Quicpro\MCP::LOG_LEVEL_INFO)
 *
 * Returns an MCP connection resource on success, FALSE on failure (exception often thrown).
 */
PHP_FUNCTION(quicpro_mcp_connect);

/*
 * PHP_FUNCTION(quicpro_mcp_close);
 * --------------------------------
 * Closes an active MCP connection resource.
 * Corresponds to `$mcpClient->close()` in PHP.
 *
 * Userland Signature:
 * bool quicpro_mcp_close(resource $mcp_connection)
 */
PHP_FUNCTION(quicpro_mcp_close);

/*
 * PHP_FUNCTION(quicpro_mcp_request);
 * ----------------------------------
 * Sends a unary (request-response) message to a specific service and method
 * on an established MCP connection. This is the workhorse for most tool calls
 * made by the PipelineOrchestrator.
 * Corresponds to `$mcpClient->sendRequest(...)` in PHP.
 *
 * Userland Signature:
 * string|false quicpro_mcp_request(
 * resource $mcp_connection,
 * string $service_name,
 * string $method_name,
 * string $request_payload_binary // Serialized by Quicpro\Proto
 * [, array $per_request_options = []] // e.g., ['timeout_ms' => 5000] for this specific request
 * )
 *
 * Returns the binary response payload on success, FALSE on failure.
 * Exceptions may be thrown for connection or protocol errors.
 */
PHP_FUNCTION(quicpro_mcp_request);

/*
 * PHP_FUNCTION(quicpro_mcp_upload_from_stream);
 * ---------------------------------------------
 * Sends data from a PHP stream resource as a client-initiated stream to an MCP service.
 * Useful for uploading large files (videos, ZIPs).
 * Corresponds to `$mcpClient->uploadStream(...)` or similar in PHP.
 *
 * Userland Signature:
 * bool quicpro_mcp_upload_from_stream(
 * resource $mcp_connection,
 * string $service_name,
 * string $method_name,
 * string $stream_identifier, // To correlate with metadata or a previous request
 * resource $php_readable_stream_resource // Readable PHP stream (e.g., from fopen)
 * [, string $initial_metadata_payload_binary = ""] // Optional initial metadata sent on the QUIC stream before data
 * )
 *
 * Returns true on successful stream completion, false on error.
 */
PHP_FUNCTION(quicpro_mcp_upload_from_stream);

/*
 * PHP_FUNCTION(quicpro_mcp_download_to_stream);
 * ---------------------------------------------
 * Initiates a request to an MCP service that is expected to respond with a data stream,
 * and writes the incoming data stream into a writable PHP stream resource.
 * Useful for downloading large files.
 *
 * Userland Signature:
 * bool quicpro_mcp_download_to_stream(
 * resource $mcp_connection,
 * string $service_name,
 * string $method_name,
 * string $request_payload_binary, // Payload to initiate the download (e.g., file ID)
 * resource $php_writable_stream_resource // Writable PHP stream (e.g., from fopen('path', 'wb'))
 * )
 *
 * Returns true on successful stream completion, false on error.
 */
PHP_FUNCTION(quicpro_mcp_download_to_stream);

/*
 * PHP_FUNCTION(quicpro_mcp_get_error);
 * -----------------------------------
 * Retrieves the last error message specific to MCP operations.
 *
 * Userland Signature:
 * string quicpro_mcp_get_error([resource $mcp_connection_optional])
 */
PHP_FUNCTION(quicpro_mcp_get_error);

#endif /* QUICPRO_MCP_H */