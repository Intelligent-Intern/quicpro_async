
Demo – Multi‑worker QUIC/HTTP‑3 echo server with **TLS session resumption**
==========================================================================

This script shows how you can scale a PHP QUIC server across CPU cores
**without** an external load‑balancer while still benefiting from 0‑RTT
TLS handshakes on subsequent connections.
 Why does it matter?
-------------------
• *Workers*: PHP veterans know the fork‑per‑request model (FPM). Here we
  fork one long‑lived worker per core – each process handles many QUIC
  connections but shares the **same UDP port** via `SO_REUSEPORT`.

• *TLS session tickets*: QUIC mandates TLS 1.3. When a client reconnects
  it can present a ticket and skip the expensive full handshake. The
  quicpro_async extension stores those tickets in a **shared‑memory LRU**
  so **any** worker can resume the session.

• *Happy scaling*: Linux distributes incoming packets across the workers
  automatically – no proxy, no sticky sessions.
 
What does the code do?
---------------------

1. Determine `$workers` (CLI arg or `nproc`).
2. Fork that many child processes (`pcntl_fork`).
3. Each child binds to `0.0.0.0:4433`, accepts QUIC handshakes and echoes
   the request path plus its worker‑ID.
4. Parent just waits – `Ctrl‑C` terminates all children.

Quick test
----------
```bash
 
  $ php examples/ticket_resumption_multiworker.php 4
  $ curl --http3 -k https://localhost:4433/foo
  echo from worker 2: /foo
  $ curl --http3 -k https://localhost:4433/bar -v | grep "TLS session"
  # should show "TLS session resumed" and echo from another worker.
  
```  
  
Extra goodies
------------
• Set `QP_TRACE=1` before launching to dump perf/QLOG events for each
  ticket lookup (`perf script` will show them).
• Pass an alternative PEM via env `CERT_FILE`, `KEY_FILE`.

```php
<?php

/**
 * Multi‑worker TLS resumption demo.
 * ------------------------------------------------
 * Forks N workers (default = number of CPU cores). Each worker runs
 * a tiny HTTP/3 echo server on the SAME UDP port using SO_REUSEPORT.
 * TLS session tickets are stored in the quicpro shared‑memory LRU
 * cache, so clients can resume across workers.
 */

use Quicpro\Config;
use Quicpro\Server;

$workers = (int)($argv[1] ?? shell_exec('nproc') ?: 4);
$port    = 4433;

$cfg = Config::new([
    'cert_file' => __DIR__ . '/../certs/server.pem',
    'key_file'  => __DIR__ . '/../certs/server.key',
    'alpn'      => ['h3'],
    'session_cache' => true,   // enable shared SHM‑LRU
]);

for ($i = 0; $i < $workers; $i++) {
    if (pcntl_fork() === 0) {
        // Child – bind to same port (SO_REUSEPORT is on by default)
        $srv = new Server('0.0.0.0', $port, $cfg);
        while ($sess = $srv->accept(50)) {
            // Basic echo: send back request path as body
            $stream = $sess->receiveRequest();
            $path   = $stream->headers()[':path'] ?? '/';
            $stream->respond(200, ['content-type' => 'text/plain']);
            $stream->sendBody("echo from worker {$i}: {$path}\n", fin:true);
            $sess->close();
        }
        exit(0);
    }
}

// Parent waits indefinitely
pcntl_wait($status);
```