# Quicpro Async: Error & Exception Reference

Welcome to the complete reference guide for all exceptions thrown by the `quicpro_async` extension. Our philosophy is to use specific, typed exceptions wherever possible. This allows you to write robust `try...catch` blocks that can react precisely to different failure modes, rather than relying on generic errors.

---

## 🏛️ Base Exception Class

All exceptions thrown by this extension inherit from a common base class. This allows you to catch any potential error from `quicpro_async` with a single catch block if needed.

### `\Quicpro\Exception\QuicproException`
* **Inherits from**: `\RuntimeException`
* **Description**: The abstract base class for all exceptions in this extension. You should not catch this directly, but rather the more specific exceptions below.

~~~php
try {
    // Any quicpro function call...
} catch (\Quicpro\Exception\QuicproException $e) {
    // This will catch any error originating from the extension.
    log_application_error("A Quicpro operation failed: " . $e->getMessage());
}
~~~

---

## ⚙️ Configuration Layer Exceptions

These exceptions are thrown during the creation of a `Quicpro\Config` object, indicating an issue with your configuration parameters.

### `\Quicpro\Exception\PolicyViolationException`
* **Inherits from**: `\Quicpro\Exception\ConfigException`
* **When it's Thrown**: When `Quicpro\Config::new()` is called with an `$options` array, but the `quicpro.allow_config_override` directive in `php.ini` is set to `false`.
* **Common Causes & Solutions**: This is a deliberate security policy. Your application is trying to override a locked configuration. **Solution**: Remove the options from your `Config::new()` call and rely on the global settings, or consult your system administrator.

### `\Quicpro\Exception\UnknownConfigOptionException`
* **Inherits from**: `\Quicpro\Exception\ConfigException`
* **When it's Thrown**: When an unrecognized key (e.g., a typo) is passed in the `$options` array to `Config::new()`.
* **Common Causes & Solutions**: A typo in the option name. **Solution**: Check the option key against this documentation for the correct spelling. The exception message will specify the invalid key.

### `\Quicpro\Exception\InvalidConfigValueException`
* **Inherits from**: `\Quicpro\Exception\ConfigException`
* **When it's Thrown**: When a valid configuration key is passed a value of the wrong data type (e.g., a string for a timeout that expects an integer).
* **Common Causes & Solutions**: Passing data of the wrong type. **Solution**: Ensure the value provided matches the expected type (e.g., `int`, `bool`, `string`, `array`).

---

## 🔌 Transport & Connection Layer Exceptions

These exceptions relate to the entire QUIC connection, from the initial handshake to timeouts and protocol errors.

### `\Quicpro\Exception\ConnectionException`
* **Inherits from**: `\Quicpro\Exception\QuicproException`
* **When it's Thrown**: This is a general base class for connection-level errors, typically when a connection cannot be established for reasons like DNS failure or routing issues.
* **Common Causes & Solutions**: Hostname not resolvable, network misconfiguration, firewall blocking the UDP port. **Solution**: Verify the hostname and port, and check firewall rules.

### `\Quicpro\Exception\TlsHandshakeException`
* **Inherits from**: `\Quicpro\Exception\ConnectionException`
* **When it's Thrown**: During the TLS handshake phase if certificate validation fails (`verify_peer` is true but the certificate is invalid/untrusted) or if there is an ALPN mismatch.
* **Common Causes & Solutions**: Peer is using a self-signed certificate, the CA is not trusted, or the advertised `alpn` protocols don't overlap. **Solution**: Ensure your `ca_file` is correct or disable `verify_peer` for trusted internal networks.

### `\Quicpro\Exception\TimeoutException`
* **Inherits from**: `\Quicpro\Exception\ConnectionException`
* **When it's Thrown**: Base class for all timeout-related errors. Catches any kind of timeout.
* **Common Causes & Solutions**: Network latency, unresponsive peer, or misconfigured timeout values.
* **Code Example**:
    ~~~php
    try {
        $response = $mcpClient->request(...);
    } catch (\Quicpro\Exception\TimeoutException $e) {
        // General handling for any timeout (idle, handshake, etc.)
        log_warning("Operation timed out: " . $e->getMessage());
        // Implement retry logic with backoff.
    }
    ~~~

### `\Quicpro\Exception\ProtocolViolationException`
* **Inherits from**: `\Quicpro\Exception\ConnectionException`
* **When it's Thrown**: When the remote peer sends a packet that violates a MUST requirement of the QUIC transport protocol (e.g., invalid packet format, incorrect reserved bits). The connection is immediately terminated.
* **Common Causes & Solutions**: A bug in the peer's QUIC implementation or a network appliance corrupting packets. **Solution**: This is a fatal connection error. Report the issue to the peer's administrator.

---

## 🌊 Stream & Frame Layer Exceptions

These errors are specific to a single stream within a connection, leaving the main connection intact.

### `\Quicpro\Exception\StreamException`
* **Inherits from**: `\Quicpro\Exception\QuicproException`
* **When it's Thrown**: A base class for errors that occur on a specific HTTP/3 or QUIC stream.
* **Common Causes & Solutions**: The stream was reset by the peer, flow control limits were exceeded, or the stream was used in an invalid state.

### `\Quicpro\Exception\StreamLimitException`
* **Inherits from**: `\Quicpro\Exception\StreamException`
* **When it's Thrown**: When attempting to open a new stream would exceed the maximum concurrent stream limit advertised by the peer.
* **Common Causes & Solutions**: The application is trying to open too many parallel requests on a single connection. **Solution**: Wait for some existing streams to finish before opening new ones, or open a new QUIC connection to distribute the load.

### `\Quicpro\Exception\FrameException`
* **Inherits from**: `\Quicpro\Exception\StreamException`
* **When it's Thrown**: When the peer sends an invalid HTTP/3 frame, such as a HEADERS frame that is too large or a malformed DATA frame.
* **Common Causes & Solutions**: The peer is sending non-compliant HTTP/3 data. **Solution**: This indicates a problem with the remote server implementation. The stream will be reset.

---

## 🚀 High-Level API Exceptions

Exceptions thrown by the high-level abstractions like the Pipeline Orchestrator, MCP, and IIBIN.

### `\Quicpro\Exception\PipelineException`
* **Inherits from**: `\Quicpro\Exception\QuicproException`
* **When it's Thrown**: During the execution of a workflow via `PipelineOrchestrator::run()`.
* **Common Causes & Solutions**: A step in the pipeline failed, an input mapping could not be resolved, or a required tool handler was not registered. **Solution**: Check the pipeline definition and ensure all required tools and data are available.

### `\Quicpro\Exception\IIBINException`
* **Inherits from**: `\Quicpro\Exception\QuicproException`
* **When it's Thrown**: During `IIBIN::encode()` or `IIBIN::decode()`.
* **Common Causes & Solutions**:
    * Encoding: The PHP data structure does not match the schema (e.g., missing a required field, wrong data type).
    * Decoding: The binary data is corrupt or does not conform to the specified schema.
    * A schema or enum name is used before it has been defined.

### `\Quicpro\Exception\MCPException`
* **Inherits from**: `\Quicpro\Exception\QuicproException`
* **When it's Thrown**: During communication with an MCP agent. This is a base class for MCP-specific errors.
* **Common Causes & Solutions**: The remote MCP agent returned an application-level error, the request payload was invalid, or the service/method name was not found.

### `\Quicpro\Exception\WebSocketException`
* **Inherits from**: `\Quicpro\Exception\QuicproException`
* **When it's Thrown**: During a WebSocket operation.
* **Common Causes & Solutions**: The WebSocket upgrade (CONNECT request) failed, a malformed WebSocket frame was received, or the connection was closed unexpectedly.

---

## 🖥️ Server & Cluster Exceptions

Exceptions related to server-side operations and the multi-process cluster.

### `\Quicpro\Exception\ServerException`
* **Inherits from**: `\Quicpro\Exception\QuicproException`
* **When it's Thrown**: During server startup or runtime.
* **Common Causes & Solutions**: Failure to bind to the specified host/port (e.g., port already in use, insufficient privileges to bind to a low port), or errors loading server-side certificates.

### `\Quicpro\Exception\ClusterException`
* **Inherits from**: `\Quicpro\Exception\QuicproException`
* **When it's Thrown**: During the operation of the `Quicpro\Cluster` supervisor.
* **Common Causes & Solutions**: Failure to fork a new worker process, errors setting worker permissions (e.g., CPU affinity, niceness), or invalid callables provided in the cluster configuration.