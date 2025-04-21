<?php
/** @generate-function-entries */
class QuicPro_Session {}
function quicpro_connect(string $host, int $port): resource {}
function quicpro_close($session): void {}
function quicpro_send_request($session, string $path, array $headers = null, string $body = ""): int {}
function quicpro_receive_response($session, int $stream_id): array {}
function quicpro_poll($session, int $timeout_ms = -1): bool {}
function quicpro_cancel_stream($session, int $stream_id): bool {}
function quicpro_export_session_ticket($session): string {}
function quicpro_import_session_ticket($session, string $ticket): bool {}
function quicpro_set_ca_file(string $ca_file): bool {}
function quicpro_set_client_cert(string $cert_pem, string $key_pem): bool {}
function quicpro_get_last_error(): string {}
function quicpro_get_stats($session): array {}
function quicpro_version(): string {}
