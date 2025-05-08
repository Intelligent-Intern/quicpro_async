<?php
/** @generate-function-entries */
declare(strict_types=1);

/*
 * ─────────────────────────────────────────────────────────────────────────────
 *  quicpro_async – user-land façade for IDEs, Psalm & PHPStan
 * ─────────────────────────────────────────────────────────────────────────────
 *  All public symbols live in this single file so that static analysers can
 *  resolve them without parsing C source.  Every body is a no-op placeholder;
 *  real work happens inside the Zend extension implemented in Rust/C.
 */

namespace Quicpro {

    /* ─────────────────────────────── OOP API ─────────────────────────────── */

    /**
     * Immutable configuration bag.
     *
     * Unknown keys raise a \InvalidArgumentException at construction time so
     * that invalid configs never travel far in the system.
     *
     * @psalm-type Options = array<string,scalar|array|bool|null>
     * @param Options $options
     */
    final class Config
    {
        public static function new(array $options = []): self
        {
            return new self; // stub
        }
    }

    /**
     * One QUIC connection including HTTP/3 and optional WebSocket streams.
     */
    final class Session
    {
        /**
         * Perform an in-place HTTP/3 → WebSocket upgrade (RFC 8441).
         *
         * Fragmentation/reassembly is handled by the engine, therefore the
         * returned resource acts like a regular PHP stream.
         *
         * @param  string $path   Request-URI path
         * @param  bool   $binary true → initial opcode = BINARY (0x2)
         * @return resource       Writable + readable stream
         */
        public function upgrade(string $path, bool $binary = false)
        {
            return fopen('php://memory', 'r+'); // stub
        }

        /** Has <path> already been upgraded to a WebSocket? */
        public function isWebSocket(string $path): bool { return false; }

        /** True once 1-RTT keys are available and handshake finished. */
        public function isConnected(): bool { return false; }

        /** Graceful CONNECTION_CLOSE (RFC 9000 §10.3). */
        public function close(): void {}
    }

    /**
     * Minimal QUIC listener used by tests and CLI tools.
     */
    final class Server
    {
        public function __construct(string $addr, int $port, Config $cfg) {}

        /**
         * Block until a client connects or the event-loop shuts down.
         * Returning null signals a clean shutdown (no error).
         */
        public function accept(): ?Session { return null; }
    }

    /* ───────────────────────── Procedural core API ───────────────────────── */

    /**
     * Establish a QUIC connection and run the cryptographic handshake.
     */
    function quicpro_connect(string $host, int $port): Session
    {
        return new Session; // stub
    }

    /**
     * Immediate CONNECTION_CLOSE without waiting for ACKs.
     */
    function quicpro_close(Session $session): void {}

    /**
     * Fire-and-forget HTTP/3 request.
     *
     * @param  array<string,string>|null $headers
     * @return int                       Stream-ID
     */
    function quicpro_send_request(
        Session $session,
        string  $path,
        ?array  $headers = null,
        string  $body    = ''
    ): int {
        return 0;
    }

    /**
     * Pull response data from the engine.
     *
     * @return array<string,mixed>|string|null
     */
    function quicpro_receive_response(Session $session, int $stream_id): array|string|null
    {
        return null;
    }

    /**
     * Drive the event-loop from user-land (alt. to libevent/AMP integration).
     */
    function quicpro_poll(Session $session, int $timeout_ms = -1): bool { return false; }

    /**
     * Abort a single HTTP/3 stream (RFC 9114 §4.2).
     */
    function quicpro_cancel_stream(Session $session, int $stream_id): bool { return false; }

    /** Export RFC 5077 session ticket for 0-RTT resumption. */
    function quicpro_export_session_ticket(Session $session): string { return ''; }

    /** Import a previously saved 0-RTT session ticket. */
    function quicpro_import_session_ticket(Session $session, string $ticket): bool { return false; }

    /** Load additional system-trust anchors. */
    function quicpro_set_ca_file(string $ca_file): bool { return false; }

    /** Configure mutual TLS credentials. */
    function quicpro_set_client_cert(string $cert_pem, string $key_pem): bool { return false; }

    /** Enable qlog tracing on $session (RFC 9002 §6). */
    function quicpro_set_qlog_path(Session $session, string $path): bool { return false; }

    /** Last engine error message – empty string if none. */
    function quicpro_get_last_error(): string { return ''; }

    /** Transport statistics according to RFC 9000 §18. */
    function quicpro_get_stats(Session $session): array { return []; }

    /** Semantic-version string of the loaded extension. */
    function quicpro_version(): string { return '0.0.0'; }

    /* ──────────────── Low-level HTTP/3 helper API ───────────────── */

    /** Streamed response body chunk (server push / big uploads). */
    function quiche_h3_send_body_chunk(Session $session, int $stream_id, string $chunk): bool { return false; }

    /** Signal “END_STREAM” once the body is complete. */
    function quiche_h3_finish_body(Session $session, int $stream_id): bool { return false; }

    /* ─────────────────────── WebSocket helper API ─────────────────────── */

    /**
     * Transmit a Ping control frame (RFC 6455 §5.5.2).
     *
     * @param  resource       $ws
     * @param  string|null    $data  ≤ 125 bytes application data
     */
    function quicpro_ws_ping($ws, ?string $data = null): bool { return false; }

    /**
     * Await the matching Pong (RFC 6455 §5.5.3).  Null → timeout.
     *
     * @param  resource $ws
     * @return string|null
     */
    function quicpro_ws_wait_pong($ws, int $timeout_ms = -1): ?string { return null; }
}

/* ───────────────────────── Global legacy wrappers ───────────────────────── */

namespace {

    use Quicpro\Session as QSession;

    function quicpro_connect(string $host, int $port): QSession
    {
        return \Quicpro\quicpro_connect($host, $port);
    }

    function quicpro_close(QSession $session): void
    {
        \Quicpro\quicpro_close($session);
    }

    function quicpro_send_request(QSession $session, string $path, ?array $headers = null, string $body = ''): int
    {
        return \Quicpro\quicpro_send_request($session, $path, $headers, $body);
    }

    function quicpro_receive_response(QSession $session, int $stream_id): array|string|null
    {
        return \Quicpro\quicpro_receive_response($session, $stream_id);
    }

    function quicpro_poll(QSession $session, int $timeout_ms = -1): bool
    {
        return \Quicpro\quicpro_poll($session, $timeout_ms);
    }

    function quicpro_cancel_stream(QSession $session, int $stream_id): bool
    {
        return \Quicpro\quicpro_cancel_stream($session, $stream_id);
    }

    function quicpro_export_session_ticket(QSession $session): string
    {
        return \Quicpro\quicpro_export_session_ticket($session);
    }

    function quicpro_import_session_ticket(QSession $session, string $ticket): bool
    {
        return \Quicpro\quicpro_import_session_ticket($session, $ticket);
    }

    function quicpro_set_ca_file(string $ca_file): bool
    {
        return \Quicpro\quicpro_set_ca_file($ca_file);
    }

    function quicpro_set_client_cert(string $cert_pem, string $key_pem): bool
    {
        return \Quicpro\quicpro_set_client_cert($cert_pem, $key_pem);
    }

    function quicpro_set_qlog_path(QSession $session, string $path): bool
    {
        return \Quicpro\quicpro_set_qlog_path($session, $path);
    }

    function quicpro_get_last_error(): string
    {
        return \Quicpro\quicpro_get_last_error();
    }

    function quicpro_get_stats(QSession $session): array
    {
        return \Quicpro\quicpro_get_stats($session);
    }

    function quicpro_version(): string
    {
        return \Quicpro\quicpro_version();
    }

    function quiche_h3_send_body_chunk(QSession $session, int $stream_id, string $chunk): bool
    {
        return \Quicpro\quiche_h3_send_body_chunk($session, $stream_id, $chunk);
    }

    function quiche_h3_finish_body(QSession $session, int $stream_id): bool
    {
        return \Quicpro\quiche_h3_finish_body($session, $stream_id);
    }

    function quicpro_ws_ping($ws, ?string $data = null): bool
    {
        return \Quicpro\quicpro_ws_ping($ws, $data);
    }

    function quicpro_ws_wait_pong($ws, int $timeout_ms = -1): ?string
    {
        return \Quicpro\quicpro_ws_wait_pong($ws, $timeout_ms);
    }
}
