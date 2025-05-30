## Overview

The `quicpro_connect()` API is the central entry point to establish a new QUIC/HTTP3 connection from PHP. This document describes its behavior, supported options, recommended best practices for low-latency connections, and all relevant usage examples and error scenarios.

---

## Function Signature

~~~C
resource quicpro_connect(
    string $host,
    int $port,
    resource $config,
    ?array $options = null
)
~~~

- **$host**: The remote server's hostname or IP address.
- **$port**: The UDP port on the remote server.
- **$config**: A `quicpro` configuration resource created by `quicpro_new_config()`.
- **$options** (optional): An associative array for advanced connection controls (see below).

Returns:  
A session resource on success, or `FALSE` on failure (with an error retrievable via `quicpro_get_last_error()`).

---

## Options Array

You may provide an optional fourth argument (`$options`) to control connection mode and address selection. Supported keys:

- **family**:
    - `"auto"` (default) — Try IPv6 first, then IPv4 after a short delay (Happy Eyeballs).
    - `"ipv4"` — Force IPv4 only.
    - `"ipv6"` — Force IPv6 only.

- **happy_eyeballs_delay_ms**:
    - Integer (default: `250`) — Delay in milliseconds before falling back from IPv6 to IPv4.
    - For ultra-low-latency or in controlled environments, set this to `0` or a small value (1-10 ms).
    - **Note:** If `"family"` is explicitly set to `"ipv4"` or `"ipv6"`, this value is ignored.


- **iface**:
    - String — Name of a network interface to bind the UDP socket (e.g., `"eth0"`). Optional.

### Example: Forcing IPv4

~~~php
$sess = quicpro_connect(
    "myhost.example.com",
    443,
    $cfg,
    ["family" => "ipv4"] // happy_eyeballs_delay_ms is ignored
);
~~~

### Example: Default "auto" mode (RFC 8305 Happy Eyeballs)

~~~php
$sess = quicpro_connect("myhost.example.com", 443, $cfg);
// or
$sess = quicpro_connect("myhost.example.com", 443, $cfg, ["family" => "auto"]);
~~~

---

## Typical Usage

1. **Create and configure the QUIC context:**
~~~php
    $cfg = quicpro_new_config([
        "verify_peer" => true,
        "alpn" => ["h3", "hq-29"]
    ]);
~~~

2. **Connect to the server:**
~~~php
    $sess = quicpro_connect("example.com", 443, $cfg, [
        "family" => "auto",
        "happy_eyeballs_delay_ms" => 10
    ]);
    if (!$sess) {
        throw new RuntimeException(quicpro_get_last_error());
    }
~~~

3. **Send requests and receive responses as usual...**

---

## Address Selection & Delay Tuning

- By default, `quicpro_connect()` uses "Happy Eyeballs" (RFC 8305):
    - Attempts IPv6 first.
    - Waits a (configurable) small number of milliseconds.
    - Falls back to IPv4 if IPv6 doesn't succeed immediately.

- **Tuning for Latency:**  
  In high-frequency trading, real-time, or containerized environments, set `happy_eyeballs_delay_ms` to `0` or a single-digit value for maximum speed.  
  Example:
~~~php
    ["family" => "auto", "happy_eyeballs_delay_ms" => 2]
~~~

- **Force Only One Family:**  
  Use `"family" => "ipv4"` or `"ipv6"` to avoid dual-stack probing.

---

## Error Handling

On failure, `quicpro_connect()` returns `FALSE`.  
Always check the return value before continuing.  
Use `quicpro_get_last_error()` to obtain the most recent error string.

Example:

~~~php
$sess = quicpro_connect("invalid.host", 443, $cfg);
if (!$sess) {
    error_log("Connect failed: " . quicpro_get_last_error());
    exit(1);
}
~~~

---

## Internals & Notes

- If you provide an `iface` in options, the UDP socket will be bound to that device before connecting.
- If your PHP is running in a container or needs deterministic routing, always specify the correct `family` and `iface`.
- For best portability, leave options as default unless you have specific performance or topology requirements.

---

## Troubleshooting

- **DNS Resolution Failure:**
    - Check that the `$host` is valid and resolvable from your PHP container or host system.
- **QUIC Handshake Errors:**
    - Make sure the remote endpoint supports QUIC/HTTP3 on the selected port.
    - Check your `quicpro_new_config()` parameters, especially ALPN.

---

## Further Reading

- [RFC 8305: Happy Eyeballs v2](https://datatracker.ietf.org/doc/html/rfc8305)
- [quiche - QUIC, HTTP/3 library](https://github.com/cloudflare/quiche)
- See also the main `README.md` for full examples.
