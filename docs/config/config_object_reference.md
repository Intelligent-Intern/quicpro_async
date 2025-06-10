# Quicpro\Config Object Reference

The `Quicpro\Config` object is a crucial component for controlling the behavior of individual QUIC sessions. It's an immutable configuration bag that allows you to fine-tune everything from security policies to performance parameters, overriding the system-wide defaults set in `php.ini`.

Creating a `Config` object is a critical step, and the extension provides specific exceptions to help you diagnose configuration errors quickly:

* **`\Quicpro\Exception\PolicyViolationException`**: Thrown if you attempt to pass any options to `Config::new()` when the `quicpro.allow_config_override` setting in `php.ini` is set to `false`. This indicates that the system administrator has locked the configuration and your code is not authorized to override it.
* **`\Quicpro\Exception\UnknownConfigOptionException`**: Thrown if you pass an unrecognized key in the `$options` array (e.g., `'foo' => 'bar'`). The exception message will clearly state which key is invalid, preventing typos.
* **`\Quicpro\Exception\InvalidConfigValueException`**: Thrown if a valid key is given a value of the wrong data type (e.g., passing a string to `max_idle_timeout_ms` which expects an integer). The message will specify the option and the expected type.

**Reference:** For a full guide on all exceptions, see the [Error and Exception Reference](./../error_and_exception_reference.md).

---

## Basic Usage

You create a configuration object by calling the static `new()` method with an associative array of options.

~~~php
<?php
// A simple configuration to enable peer verification
$config = Quicpro\Config::new([
    'verify_peer' => true,
]);

// Use this config to establish a new session
$session = new Quicpro\Session('api.example.com', 443, $config);
~~~

---

## Parameter Reference

### 🔐 Security & TLS

These options control the security and TLS-related aspects of a connection.

#### `verify_peer`
* **Type**: `boolean`
* **Default**: `false`
* **Description**: Enables or disables TLS certificate validation for the remote peer. This is the most important security setting. It should **always be `true`** when connecting to external or untrusted servers to prevent man-in-the-middle attacks.
* **Beginner Example**:
    ~~~php
    // Always verify when connecting to a public API
    $config = Quicpro\Config::new(['verify_peer' => true]);
    ~~~
* **Expert Example**:
    ~~~php
    // Disable verification ONLY for a trusted, internal service with a self-signed cert
    $config = Quicpro\Config::new(['verify_peer' => false]);
    ~~~

#### `ca_file`
* **Type**: `string`
* **Default**: `""` (Uses system's default trust store)
* **Description**: Specifies the path to a custom Certificate Authority (CA) bundle file in PEM format. This is useful if you need to trust a private or corporate CA.
* **Beginner Example**:
    ~~~php
    // Rely on the operating system's trusted CAs
    $config = Quicpro\Config::new(['verify_peer' => true]);
    ~~~
* **Expert Example**:
    ~~~php
    // Use a specific CA bundle for a corporate environment
    $config = Quicpro\Config::new([
        'verify_peer' => true,
        'ca_file' => '/etc/ssl/certs/my_corp_ca.pem',
    ]);
    ~~~

#### `cert_file` & `key_file`
* **Type**: `string`
* **Default**: `""`
* **Description**: Paths to the client's public certificate chain and private key files (PEM format). These are used for **Mutual TLS (mTLS)**, where the client must present its own certificate to authenticate itself to the server. Both must be provided together.
* **Beginner Example**:
    ~~~php
    // Connect to a service that requires mTLS authentication
    $config = Quicpro\Config::new([
        'verify_peer' => true,
        'cert_file' => './certs/client.crt',
        'key_file'  => './certs/client.key',
    ]);
    ~~~

#### `alpn`
* **Type**: `array`
* **Default**: `['h3']`
* **Description**: An array of strings specifying the **Application-Layer Protocol Negotiation** protocols to advertise to the peer, in order of preference. The server will select the first protocol from this list that it also supports.
* **Beginner Example**:
    ~~~php
    // Standard configuration for HTTP/3
    $config = Quicpro\Config::new(['alpn' => ['h3']]);
    ~~~
* **Expert Example**:
    ~~~php
    // Advertise support for a custom internal protocol as the first choice
    $config = Quicpro\Config::new(['alpn' => ['my-rpc/2.1', 'h3']]);
    ~~~

---

### 🚀 Performance & Flow Control

Tune how the connection handles data flow and adapts to network conditions.

#### `initial_max_data`
* **Type**: `integer`
* **Default**: Engine-defined value (e.g., `1048576`)
* **Description**: The initial connection-level flow control window in bytes. This is the total amount of data you can send before receiving an acknowledgment and a window update from the peer. Increasing this can improve throughput for high-bandwidth connections but also increases memory usage.
* **Expert Example**:
    ~~~php
    // Increase the connection window for a high-throughput data transfer task
    $config = Quicpro\Config::new(['initial_max_data' => 4 * 1024 * 1024]); // 4MB
    ~~~

#### `initial_max_stream_data_...`
* **Types**: `initial_max_stream_data_bidi_local`, `initial_max_stream_data_bidi_remote`, `initial_max_stream_data_uni`
* **Type**: `integer`
* **Default**: Engine-defined values
* **Description**: These parameters control the initial **per-stream** flow control window for different stream types. Tuning these is an advanced technique to manage memory on a more granular level.
* **Expert Example**:
    ~~~php
    // Allocate a large buffer for a local bidirectional stream (e.g., a file upload)
    $config = Quicpro\Config::new(['initial_max_stream_data_bidi_local' => 2 * 1024 * 1024]); // 2MB
    ~~~

#### `cc_algorithm`
* **Type**: `string`
* **Default**: `"cubic"`
* **Description**: Specifies the **Congestion Control** algorithm to use. This algorithm determines how aggressively the connection sends data to avoid overwhelming the network.
  * `"cubic"`: A modern, well-rounded algorithm suitable for most internet conditions.
  * `"reno"`: An older, less aggressive algorithm, sometimes useful for network diagnostics.
* **Expert Example**:
    ~~~php
    // Switch to Reno for specific network testing or compatibility reasons
    $config = Quicpro\Config::new(['cc_algorithm' => 'reno']);
    ~~~

---

### ⏱️ Timeouts & Timers

Configure how the connection handles idle periods and timeouts.

#### `max_idle_timeout_ms`
* **Type**: `integer`
* **Default**: Engine-defined value (e.g., `10000`)
* **Description**: The maximum idle timeout in milliseconds. If no packets are sent or received for this duration, the connection will be considered dead and closed. The actual timeout is the minimum of the value advertised by the client and the server.
* **Beginner Example**:
    ~~~php
    // Set a short 5-second idle timeout
    $config = Quicpro\Config::new(['max_idle_timeout_ms' => 5000]);
    ~~~
* **Expert Example**:
    ~~~php
    // Set a long 5-minute timeout for a WebSocket connection with infrequent keep-alives
    $config = Quicpro\Config::new(['max_idle_timeout_ms' => 300000]);
    ~~~

---

### 📈 Logging & Debugging

Options to aid in diagnostics and debugging connection issues.

#### `qlog_path`
* **Type**: `string`
* **Default**: `""` (disabled)
* **Description**: Enables **qlog** logging for the session and writes the log to the specified directory. qlog is a standardized JSON-based format for logging QUIC events, invaluable for debugging transport-layer issues with tools like `qvis`.
* **Expert Example**:
    ~~~php
    // Log all connection events for a specific session to a directory
    $logDir = '/var/log/qlogs/' . bin2hex(random_bytes(8));
    if (!is_dir($logDir)) mkdir($logDir, 0755, true);
    $config = Quicpro\Config::new(['qlog_path' => $logDir]);
    ~~~

---

## ⚡ Advanced & Expert Settings

**Warning**: These settings can have significant performance and stability implications. Use with caution.

#### `disable_active_migration`
* **Type**: `boolean`
* **Default**: `false`
* **Description**: Disables connection migration. When `false`, a QUIC connection can survive a network change (e.g., switching from Wi-Fi to cellular). Setting this to `true` will cause the connection to drop on network path changes but can simplify server-side logic in environments where client IPs are expected to be static.

#### `busy_poll_duration_us`
* **Type**: `integer`
* **Default**: `0`
* **Description**: For internal connections on dedicated networks. Tells the I/O event loop to "busy-poll" for the specified duration in microseconds before yielding the CPU. This can reduce latency by immediately processing incoming packets, but at the cost of significantly higher CPU usage. Only use this for extreme low-latency requirements.

---

## Full Examples

* For a complete, runnable script demonstrating a simple and safe configuration, see:
  [**`examples/config/beginner.php`**](./../../examples/config/beginner.php)

* For an advanced configuration tuned for a low-latency, high-performance internal service, see:
  [**`examples/config/expert.php`**](./../../examples/config/expert.php)