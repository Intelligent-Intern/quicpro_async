# `quicpro_async`: `php.ini` Configuration Guide

This document provides a comprehensive reference for all `php.ini` directives available in the `quicpro_async` extension. These settings establish system-wide defaults, allowing administrators to tune the core behavior of the engine. Most of these values can be overridden at runtime within your PHP code for more granular control.

---

## 🚀 Cluster Supervisor Settings

These directives control the behavior of the C-native multi-process supervisor (`Quicpro\Cluster`), which is the heart of the extension's vertical scaling capabilities.

### `quicpro.workers`
* **Type**: `integer`
* **Default**: `0`
* **Description**: Defines the default number of worker processes to spawn. A worker is an independent PHP process forked from the master to handle tasks in parallel. The special value `0` enables **auto-detection**, where the number of workers will match the number of available CPU cores on the machine. This is the recommended setting for automatically maximizing hardware utilization.

### `quicpro.grace_timeout`
* **Type**: `integer`
* **Default**: `30`
* **Description**: Specifies the "grace period" in seconds. When the cluster is instructed to shut down, each worker process first receives a `SIGTERM` signal. This timeout defines how long the master process will wait for workers to finish their current tasks and exit cleanly. If a worker does not exit within this period, it will be forcefully terminated with a `SIGKILL` signal.

### `quicpro.max_fd_per_worker`
* **Type**: `integer`
* **Default**: `8192`
* **Description**: Sets the maximum number of open file descriptors (`RLIMIT_NOFILE`) for each worker process. This is a critical setting for high-concurrency applications that manage thousands of network connections or file handles, as it prevents "Too many open files" errors.

### `quicpro.usleep_usec`
* **Type**: `integer`
* **Default**: `0`
* **Description**: A hint for a worker's main loop, specifying how many microseconds to sleep when idle. A value greater than `0` reduces CPU usage when there is no work to be done. A value of `0` enables **busy-polling**, where the loop spins without sleeping. This can dramatically reduce latency for receiving new events but will consume more CPU. Use `0` only in ultra-low-latency environments.

---

## ⚙️ Server & Connection Defaults

These settings define the default behavior for server applications built with the extension.

### `quicpro.host`
* **Type**: `string`
* **Default**: `"0.0.0.0"`
* **Description**: The default IP address that server applications will bind to when listening for incoming connections. `"0.0.0.0"` means the server will accept connections on all available network interfaces.

### `quicpro.port`
* **Type**: `integer`
* **Default**: `4433`
* **Description**: The default UDP port that server applications will listen on.

### `quicpro.max_sessions`
* **Type**: `integer`
* **Default**: `65536`
* **Description**: The maximum number of concurrent QUIC sessions (i.e., unique client connections) that a single server process will manage. This acts as a crucial safeguard against resource exhaustion from connection floods or DoS attacks.

### `quicpro.maintenance_mode`
* **Type**: `boolean`
* **Default**: `0` (false)
* **Description**: A global flag that can be read by your application to enable a maintenance mode. When enabled, your application logic could, for example, refuse new connections or return a "503 Service Unavailable" status, facilitating graceful deployments and updates.

---

## 🛡️ Security & Policy Settings

This section contains meta-settings that control the overall behavior and security posture of the extension.

### `quicpro.allow_config_override`
* **Type**: `boolean`
* **Default**: `1` (true)
* **Description**: This is a master switch that governs configuration precedence. When set to `true` (the default), settings passed in the `$options` array to `Quicpro\Config::new()` **can override** the values set in `php.ini`. This provides maximum flexibility for developers. When set to `false`, the `php.ini` settings are considered **final and cannot be overridden** by application code. Setting this to `false` is recommended for production environments where system administrators need to enforce strict security policies (e.g., forcing TLS peer verification) or performance parameters.

---

## 🔐 TLS Configuration

Default paths for TLS certificates used for new connections.

### `quicpro.ca_file`
* **Type**: `string`
* **Default**: `""` (empty)
* **Description**: The absolute path to a PEM-formatted Certificate Authority (CA) bundle file. This file is used by the client to **verify the identity of the remote server** during the TLS handshake.

### `quicpro.client_cert`
* **Type**: `string`
* **Default**: `""` (empty)
* **Description**: The path to a PEM-formatted client certificate file. This is used for **Mutual TLS (mTLS)**, where the client must also present a certificate to authenticate itself to the server.

### `quicpro.client_key`
* **Type**: `string`
* **Default**: `""` (empty)
* **Description**: The path to the client's private key file (PEM-format) corresponding to the `quicpro.client_cert`.

---

## ⚡ Session Resumption (0-RTT)

Configure the powerful TLS Session Ticket system to enable zero-round-trip (0-RTT) reconnections, dramatically reducing latency for returning clients.

### `quicpro.session_mode`
* **Type**: `integer`
* **Default**: `0` (AUTO)
* **Description**: Defines the default strategy for storing and reusing session tickets.
  * `0`: **AUTO** - The extension automatically selects the best strategy based on the runtime environment (e.g., uses Shared Memory after a `fork()`).
  * `1`: **LIVE** - The ticket is only held in the memory of the live connection object. Suitable for long-running, single-process applications.
  * `2`: **TICKET** - The PHP application is responsible for manually exporting (`quicpro_export_session_ticket`) and importing tickets, for example by using a cache like APCu or Redis.
  * `3`: **WARM** - Optimized for serverless (FaaS) environments where a container may be "warm" and reused for multiple invocations.
  * `4`: **SHM_LRU** - Uses a high-performance, lock-free ring buffer in **Shared Memory**. This is the ideal mode for the `Quicpro\Cluster` supervisor, as it allows all forked workers to share tickets seamlessly.

### `quicpro.shm_size`
* **Type**: `integer`
* **Default**: `131072` (128 KiB)
* **Description**: The total size in bytes for the shared memory ticket ring buffer, used when `quicpro.session_mode` is set to `4` (SHM_LRU). The default size of 128 KiB can hold approximately **120-130 session tickets**.

### `quicpro.shm_path`
* **Type**: `string`
* **Default**: `""` (empty)
* **Description**: The name of the shared memory object created via `shm_open()`. This is useful if you run multiple, independent `quicpro_async` applications on the same machine and need to prevent them from sharing their ticket caches. If left empty, a default path like `/quicpro_ring` is used.

---

## 📊 Metrics & Observability

Settings for monitoring the extension's performance and health.

### `quicpro.metrics_enabled`
* **Type**: `boolean`
* **Default**: `1` (true)
* **Description**: If enabled, the extension exposes an HTTP endpoint with internal performance counters in a format compatible with monitoring systems like **Prometheus**.

### `quicpro.metrics_port`
* **Type**: `integer`
* **Default**: `9091`
* **Description**: The TCP port on which the metrics server will listen when `quicpro.metrics_enabled` is true.