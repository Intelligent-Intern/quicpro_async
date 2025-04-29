# Async I/O and Parallel Execution with *quicpro_async*
A detailed guide explaining how non-blocking QUIC sockets, PHP Fibers and fork-per-core workers combine to deliver low-latency and full CPU utilisation.  

---

## 1 · How the pieces fit

~~~
             +----------------------+                          +----------------------+
             |   Process 1          |                          |   Process 2          |
             |  (CPU core 0)        |                          |  (CPU core 1)        |
             |   epoll loop         |                          |   epoll loop         |
             |  Fiber A  Fiber B    |   shared UDP :4433 port  |  Fiber C  Fiber D    |
             +----------------------+                          +----------------------+
                    ↑                                                 ↑
                    |            shared TLS tickets (SHM-LRU)         |
                    └───────────── 0-RTT resumes across workers ──────┘
~~~

* **One process** handles thousands of sockets concurrently with Fibers.
* **Several processes** run in parallel on different CPU cores.
* The shared-memory ticket ring lets any worker resume any client, so zero-RTT never breaks.

---

## 2 · Creating an async client

Rules: poll inside every receive loop, use short time-outs, close the session.

~~~
function fetch(string $path): string {
    $s  = quicpro_connect('example.org', 443);
    $id = quicpro_send_request($s, $path);
    while (!$r = quicpro_receive_response($s, $id)) {
        quicpro_poll($s, 5);       // yields ≤ 5 ms
    }
    quicpro_close($s);
    return $r['body'];
}

$jobs   = ['/a', '/b', '/c'];
$fibers = array_map(fn($p) => new Fiber(fn() => fetch($p)), $jobs);
foreach ($fibers as $f) $f->start();
foreach ($fibers as $f) echo $f->getReturn(), PHP_EOL;
~~~

---

## 3 · Single-process async server

~~~
use Quicpro\Config;
use Quicpro\Server;
use Quicpro\Session;
use Fiber;

$cfg = Config::new([
    'cert_file' => 'cert.pem',
    'key_file'  => 'cert.pem',
    'alpn'      => ['h3'],
]);

$srv = new Server('[::]', 4433, $cfg);

while ($sess = $srv->accept(100)) {
    (new Fiber(fn() => handle($sess)))->start();
}

function handle(Session $s): void {
    $req    = $s->receiveRequest();
    $path   = $req->headers()[':path'] ?? '/';
    $stream = $req->stream();
    $stream->respond(200, ['content-type' => 'text/plain']);
    $stream->sendBody("echo $path\n", fin: true);
    $s->close();
}
~~~

---

## 4 · Fork-per-core server (parallelism + Fibers)

~~~
use Quicpro\Config;
use Quicpro\Server;
use Fiber;

$workers = (int) trim(shell_exec('nproc')) ?: 4;
$cfg = Config::new([
    'cert_file'     => 'cert.pem',
    'key_file'      => 'cert.pem',
    'alpn'          => ['h3'],
    'session_cache' => true          // shared ticket ring
]);

for ($i = 0; $i < $workers; $i++) {
    if (pcntl_fork() === 0) { worker($i, 4433, $cfg); exit; }
}
pcntl_wait($status);

function worker(int $id, int $port, Config $cfg): void {
    $srv = new Server('0.0.0.0', $port, $cfg);
    echo "[worker $id] ready\n";
    while ($s = $srv->accept(100)) (new Fiber(fn() => handle($s, $id)))->start();
}

function handle($sess, int $wid): void {
    $req    = $sess->receiveRequest();
    $stream = $req->stream();
    $stream->respond(200, ['content-type' => 'text/plain']);
    $stream->sendBody("hello from $wid\n", fin: true);
    $sess->close();
}
~~~

---

## 5 · Error handling

A full catalogue of exceptions, warnings, error constants and the appropriate recovery strategy is maintained in **ERROR_AND_EXCEPTION_REFERENCE.md**.  
Consult that file whenever a call throws or returns `false`; it contains ready-to-paste `try / catch` patterns and decoding of `quicpro_get_last_error()` values.

---

## 6 · Performance tuning knobs

| Knob                       | Default | When to change                                        |
|----------------------------|---------|-------------------------------------------------------|
| Environment QUICPRO_BUSY_POLL | off     | spin microseconds before yielding (ultra-low latency) |
| Environment QUICPRO_XDP       | empty   | attach AF_XDP socket for zero-copy                    |
| Ini quicpro.shm_size          | 65536   | raise for many unique clients per second              |
| Config max_idle_ms            | 30000   | lower for faster resource cleanup                     |

Busy-poll example

~~~
putenv('QUICPRO_BUSY_POLL=50');
putenv('QUICPRO_XDP=ens6');
~~~

---

## 7 · Troubleshooting flow

1. Use curl --http3 -v and check for “TLS session resumed”.
2. If absent, run php -i | grep quicpro.shm_size to confirm cache size.
3. Increase shm_size or export / import tickets per request in FPM.
4. Enable QP_TRACE=1 and inspect perf or strace output.

---

## 8 · Summary

* Fibers provide thousands of concurrent sessions inside one PHP process.
* Forked workers distribute work to every CPU core; the shared ticket ring preserves 0-RTT across processes.
* Network I/O is fully non-blocking in C; PHP code only orchestrates Fibers.
* Error details and recovery recipes live in ERROR_AND_EXCEPTION_REFERENCE.md.
* Main tuning levers: shm_size for ticket capacity and poll time-outs.

With these patterns a PHP application matches Go-level concurrency and saturates modern multi-core servers — without leaving PHP.
