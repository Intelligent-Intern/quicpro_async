<?php
/** @generate-function-entries */

namespace Quicpro;

/* ----------  OOP API  ---------- */

final class Config
{
    public static function new(array $options = []): self {}
}

final class Session
{
    public function upgrade(string $path) {}                         // WebSocket upgrade
    public function isWebSocket(string $path): bool {}               // verify WS route
    public function close(): void {}                                 // graceful shutdown
}

final class Server
{
    public function __construct(string $addr, int $port, Config $cfg) {}
    public function accept(): ?Session {}                            // accept incoming QUIC session
}

/* ----------  Procedural API  ---------- */

function quicpro_connect(string $host, int $port): resource {}
function quicpro_close($session): void {}

function quicpro_send_request(
    $session,
    string $path,
    ?array $headers = null,
    string $body = ''
): int {}

function quicpro_receive_response($session, int $stream_id): array|string|null {}
function quicpro_poll($session, int $timeout_ms = -1): bool {}
function quicpro_cancel_stream($session, int $stream_id): bool {}

function quicpro_export_session_ticket($session): string {}
function quicpro_import_session_ticket($session, string $ticket): bool {}

function quicpro_set_ca_file(string $ca_file): bool {}
function quicpro_set_client_cert(string $cert_pem, string $key_pem): bool {}
function quicpro_set_qlog_path($session, string $path): bool {}

function quicpro_get_last_error(): string {}
function quicpro_get_stats($session): array {}

function quicpro_version(): string {}

/* ----------  Low-level HTTP/3 helpers  ---------- */

function quiche_h3_send_body_chunk($session, int $stream_id, string $chunk): bool {}
function quiche_h3_finish_body($session, int $stream_id): bool {}
