<?php

use QuicProAsync\QuicServer;
use QuicProAsync\QuicConnection;
use QuicProAsync\QuicStream;

require_once __DIR__ . '/../../../vendor/autoload.php';

$host = getenv('QUIC_DEMO_HOST') ?: '0.0.0.0';
$port = getenv('QUIC_DEMO_PORT') ?: 4433;
$retryPort = getenv('QUIC_RETRY_PORT') ?: 4434;
$certFile = getenv('QUIC_DEMO_CERT') ?: '/workspace/var/traefik/certificates/local-cert.pem';
$keyFile = getenv('QUIC_DEMO_KEY') ?: '/workspace/var/traefik/certificates/local-key.pem';

$server = new QuicServer([
    'host' => $host,
    'port' => $port,
    'cert' => $certFile,
    'key' => $keyFile,
    'retry_port' => $retryPort,
    'alpn' => ['hq-interop', 'quic-http', 'hq-29'],
]);

echo "QUIC demo server running on {$host}:{$port}\n";

while (true) {
    $connection = $server->accept();
    if (!$connection instanceof QuicConnection) {
        continue;
    }

    while ($stream = $connection->acceptStream()) {
        if (!$stream instanceof QuicStream) {
            continue;
        }

        $data = $stream->read();
        if ($data !== false) {
            $stream->write("echo: " . $data);
        }
        $stream->close();
    }

    $connection->close();
}
