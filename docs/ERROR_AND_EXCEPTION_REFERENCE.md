# QuicPro Async – Error-Handling Reference

This document lists **all runtime exceptions, warnings, PHP errors, and error-code constants** exposed by the *quicpro_async* extension.

---

## 1 · Exception Classes

| Class name | Thrown when … |
|------------|---------------|
| `QuicConnectionException` | a QUIC connection cannot be created or maintained |
| `QuicStreamException` | stream creation, read, write, or shutdown fails |
| `QuicTlsHandshakeException` | TLS 1.3 handshake fails (invalid certificate, timeout, etc.) |
| `QuicTimeoutException` | any operation exceeds its deadline |
| `QuicTransportException` | QUIC protocol violation (e.g. invalid frame, version mismatch) |
| `QuicServerException` | server startup or runtime failure (port already in use, bind error) |
| `QuicClientException` | client-side connection or response failure |
| `QuicSocketException` | generic network/socket issue (unreachable, ECONNREFUSED, etc.) |

### Usage example
~~~php
try {
    $sess = quicpro_connect('bad.host', 443);
} catch (QuicConnectionException $e) {
    error_log("connect failed: " . $e->getMessage());
}
~~~

---

## 2 · Runtime Warnings

| Warning constant | Situation |
|------------------|-----------|
| `QuicWarningInvalidParameters` | invalid options provided to a function / constructor |
| `QuicWarningConnectionSlow` | RTT or handshake takes unusually long; timeout imminent |
| `QuicWarningDeprecatedFeature` | caller uses a soon-to-be-removed API flag |
| `QuicWarningPartialData` | a datagram / stream frame was only partially processed |

Warnings are emitted with `trigger_error(..., E_WARNING)` and **do not throw**.

---

## 3 · PHP Error Messages

| Severity | Example message |
|----------|-----------------|
| `E_WARNING` | `Connection handshake took too long` |
| `E_WARNING` | `Received unknown QUIC frame type 0x1f` |
| `E_ERROR`   | `Unable to initialize QUIC context` |
| `E_ERROR`   | `TLS handshake failed: invalid certificate` |

`E_ERROR` terminates the request; `E_WARNING` can be silenced or converted to an exception via `ErrorException`.

---

## 4 · Error-Code Constants

| Constant | Value | Meaning |
|----------|-------|---------|
| `QUIC_ERROR_OK` | `0` | success |
| `QUIC_ERROR_TIMEOUT` | `1` | operation timed out |
| `QUIC_ERROR_INVALID_STATE` | `2` | function called in wrong session/stream state |
| `QUIC_ERROR_CONNECTION_LOST` | `3` | connection aborted unexpectedly |
| `QUIC_ERROR_HANDSHAKE_FAILED` | `4` | TLS handshake error |
| `QUIC_ERROR_STREAM_ERROR` | `5` | unrecoverable stream failure |
| `QUIC_ERROR_TRANSPORT_ERROR` | `6` | QUIC transport-level protocol error |
| `QUIC_ERROR_INTERNAL` | `7` | internal failure (e.g. OOM) |
| `QUIC_ERROR_UNSUPPORTED_VERSION` | `8` | QUIC version not supported by peer |

Return-style APIs (e.g. `quicpro_poll()`) expose these codes via `quicpro_get_last_error()`.

---

## 5 · Quick Reference Cheat-Sheet

* **Always catch** the eight exception classes for robust error handling.
* **Monitor warnings** in logs to spot slow joins, bad parameters, and deprecated flags.
* **Treat `E_ERROR`** as fatal; design graceful degradation around `E_WARNING`.
* **Map numeric codes** from `quicpro_get_last_error()` to the table above.

~~~php
$sess = quicpro_connect('api.internal', 443);

$id   = quicpro_send_request($sess, '/data');
while (!quicpro_receive_response($sess, $id)) {
    if (!quicpro_poll($sess, 10)) {
        throw new RuntimeException('poll failed: ' . quicpro_get_last_error());
    }
}
~~~
