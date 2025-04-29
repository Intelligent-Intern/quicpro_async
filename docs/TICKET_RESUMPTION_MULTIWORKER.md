# TLS Session Resumption & Multi-Worker Scaling
**How to build a modern, fast, and scalable QUIC / HTTP-3 backend in PHP – step by step, with zero-RTT reconnects and no external load-balancer.**

This document starts with the very first handshake, explains what a *session ticket* is, and then walks through five practical retention strategies – from single-process scripts to fork-per-core servers.  
Every section includes copy-and-paste examples, plain-English background, and troubleshooting notes. Only basic PHP and cURL experience required.

---

## 0 · The problem we solve

### 0 .1 Why each fresh handshake steals time
A QUIC + TLS 1.3 handshake adds one extra round-trip before PHP sees the request. On a 40 ms mobile link that delay is 40 ms you could avoid for every reconnect.

### 0 .2 Session tickets in one sentence
The server ends the handshake with an encrypted *resume-me-later* blob.  
The client stores that blob and sends it in the very first packet next time. Both sides can then start encrypted application data immediately – this is the famous zero-RTT.

### 0 .3 Why many PHP workers break tickets
With classic FPM the kernel pins each UDP flow to a single worker. A ticket issued by worker A is useless when the next packet lands on worker B.  
The **quicpro_async** extension fixes this with a shared-memory LRU ring buffer: every worker can resume every client. Linux fans packets out automatically via *SO_REUSEPORT* – no proxy, no sticky sessions.

---

## 1 · Five practical ways to keep a session alive

| Nr | Mode name             | Best for …                              | How it works in practice                                      |
|:--:|-----------------------|-----------------------------------------|---------------------------------------------------------------|
| 1  | **Live object**       | long-running CLI apps, ReactPHP loops   | keep the `$sess` resource in a global variable                |
| 2  | **Ticket cache**      | classic FPM, short CLI jobs             | export ticket → store in APCu / Redis / file → import later   |
| 3  | **Warm container**    | AWS Lambda, Google Cloud Run            | keep `$sess` in file scope; reused container = free zero-RTT  |
| 4  | **Shared-memory LRU** | fork()-based multi-worker on one host   | compiled-in shm cache; workers read the same ring             |
| 5  | **Stateless LB**      | many hosts behind anycast / layer-4 LB  | client keeps ticket (mode 2); any edge can resume it          |

Quick rule of thumb  
• one PHP process → live object • several FPM children → ticket cache • forked workers → shared-memory LRU

---

## 2 · Compiling and configuring the shared ticket cache

### 2 .1 Build

The task scheduler and shared-memory ticket ring are **enabled by default**. No special flags are required.

~~~bash
phpize
./configure
make -j$(nproc)
sudo make install
~~~

### 2 .2 php.ini switches (environment variables still work but are optional)

~~~ini
; /etc/php/8.3/mods-available/quicpro_async.ini
extension               = quicpro_async.so

; size of the ticket ring (bytes); 128 kB ≈ 120 tickets
quicpro.shm_size        = 131072

; custom shm_open() name when several independent servers run
quicpro.shm_path        = /quicpro_ring

; choose retention mode: 0=AUTO 1=LIVE 2=TICKET 3=WARM 4=SHM_LRU
quicpro.session_mode    = 0
~~~

### 2 .3 Session-mode constants available in PHP

~~~text
QUICPRO_SESSION_AUTO        (0)
QUICPRO_SESSION_LIVE        (1)
QUICPRO_SESSION_TICKET      (2)
QUICPRO_SESSION_WARM        (3)
QUICPRO_SESSION_SHM_LRU     (4)
~~~

Override php.ini for a single server:

~~~php
$cfg = Quicpro\Config::new([
    'session_mode' => QUICPRO_SESSION_TICKET,
    'alpn'         => ['h3'],
]);
~~~

AUTO chooses the smartest mode at runtime:

* live object when the process never exits
* ticket cache under FPM
* warm when a FaaS variable is detected
* shm-LRU after fork() when shm cache is available

---

## 3 · Hands-on: zero-RTT echo server with one worker per core

The script below forks *N* children (default = number of CPU cores), binds them all to UDP :4433, and resumes tickets across workers. Each worker processes sessions inside a Fiber so long polls never block new handshakes.

***examples/ticket_resumption_multiworker.php***

~~~php
<?php

declare(strict_types=1);

use Quicpro\Config;
use Quicpro\Server;
use Quicpro\Session;
use Fiber;

/* choose worker count */
$workers = (int) ($argv[1] ?? trim(shell_exec('nproc'))) ?: 4;
$port    = 4433;

/* shared config */
$cfg = Config::new([
    'cert_file'     => __DIR__ . '/../certs/server.pem',
    'key_file'      => __DIR__ . '/../certs/server.key',
    'alpn'          => ['h3'],
    'session_cache' => true,        // shared tickets
]);

echo "Spawning {$workers} workers on UDP :{$port}\n";

/* master forks */
for ($i = 0; $i < $workers; $i++) {
    if (pcntl_fork() === 0) { worker($i, $port, $cfg); exit; }
}
pcntl_wait($status);

/* worker loop */
function worker(int $wid, int $port, Config $cfg): void
{
    $srv = new Server('0.0.0.0', $port, $cfg);   /* SO_REUSEPORT implicit */
    echo "[worker {$wid}] ready\n";

    while ($sess = $srv->accept(100)) {
        (new Fiber(fn() => handle($sess, $wid)))->start();
    }
}

function handle(Session $s, int $wid): void
{
    $req    = $s->receiveRequest();
    $path   = $req->headers()[':path'] ?? '/';
    $stream = $req->stream();
    $stream->respond(200, ['content-type' => 'text/plain']);
    $stream->sendBody("echo from worker {$wid}: {$path}\n", fin: true);
    $s->close();
}
~~~

### 3 .1 Run

~~~bash
sudo setcap cap_net_bind_service=+ep $(command -v php)
php examples/ticket_resumption_multiworker.php 4
~~~

### 3 .2 Observe zero-RTT across workers

~~~bash
curl --http3 -k https://localhost:4433/foo
curl --http3 -k https://localhost:4433/bar -v | grep session
# should print “TLS session resumed” and a worker ID that may differ
~~~

---

## 4 · Where each mode shines in real projects

* API gateway with dozens of upstream sessions → live object
* Classical FPM web app → ticket cache (APCu is enough)
* Lambda function with 200 ms bursts → warm container
* Chat backend on a 16-core box → shared-memory LRU
* Anycast edge nodes worldwide → client ticket cache + stateless servers

You can mix modes; they work independently.

---

## 5 · Troubleshooting

A full catalogue of exceptions, warnings, error constants and recovery recipes is maintained in **ERROR_AND_EXCEPTION_REFERENCE.md**. Consult that file whenever a call throws or returns *false*.

---

## 6 · Cheat-sheet to pin on your monitor

~~~text
# compile (ticket ring is built-in)
phpize
./configure
make -j$(nproc)
sudo make install

# php.ini quick setup
quicpro.shm_size     = 131072
quicpro.session_mode = 4        ; force shared-memory LRU

# run eight workers on UDP :4433
php examples/ticket_resumption_multiworker.php 8
~~~

Now you know what session tickets are, why they cut latency, and how to keep them alive in every PHP deployment scenario – from a single CLI script to an eight-core QUIC server that scales without a load-balancer. Enjoy your instant handshakes. 🚀
