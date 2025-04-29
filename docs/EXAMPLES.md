# quicpro_async – Usage Examples

---


## 1 · Single **GET** request (synchronous)

**Why?**  
Health-checks, one-shot CLI scripts, cron jobs, or any task where you
need *exactly one* HTTP/3 request and do not care about overlap or
latency hiding.

~~~php
$sess   = quicpro_connect('example.org', 443);
$stream = quicpro_send_request($sess, '/');

while (!$resp = quicpro_receive_response($sess, $stream)) {
    quicpro_poll($sess, 20);                  // blocks ≤ 20 ms per loop
}

echo $resp['body'];
quicpro_close($sess);
~~~

---

## 2 · Parallel GETs with Fibers (concurrent client)

**Why?**  
Scrape three pages in parallel during a CMS warm-up, pre-render a trio
of micro-frontends, or fetch several API endpoints concurrently in one
CLI process.

~~~php
function fetch(string $path): string {
    $s  = quicpro_connect('example.org', 443);
    $id = quicpro_send_request($s, $path);
    while (!$r = quicpro_receive_response($s, $id)) quicpro_poll($s, 5);
    quicpro_close($s);
    return $r['body'];
}

$jobs   = ['/a', '/b', '/c'];
$fibers = array_map(fn($p) => new Fiber(fn() => fetch($p)), $jobs);
foreach ($fibers as $f) $f->start();
foreach ($fibers as $f) echo $f->getReturn(), PHP_EOL;
~~~

---

## 3 · Streaming **POST** upload (chunked body)

**Why?**  
Send a multi-gigabyte file from disk to an object-storage gateway without
loading it entirely into memory. The body is streamed in 16 kB chunks,
and the loop never blocks longer than one system call.

~~~php
$s   = quicpro_connect('uploader.local', 8443);
$sid = quicpro_send_request(
    $s, '/upload', headers: ['content-type' => 'application/octet-stream']
);

$file = fopen('dump.raw', 'rb');
while (!feof($file)) {
    quiche_h3_send_body_chunk($s, $sid, fread($file, 16 * 1024));
    quicpro_poll($s, 0);                      // non-blocking
}
quiche_h3_finish_body($s, $sid);

while (!$r = quicpro_receive_response($s, $sid)) quicpro_poll($s, 10);
print_r($r);
~~~

---

## 4 · WebSocket chat client (HTTP-3 CONNECT)

**Why?**  
Build a thin chat front-end, subscribe to an event stream, or interact
with a push API where the server talks back at arbitrary times.

~~~php
$sess = quicpro_connect('chat.example', 443);
$ws   = $sess->upgrade('/chat');              // RFC 9220 CONNECT → WS

fwrite($ws, "hello from PHP\n");
while (!feof($ws)) echo "Server: ", fgets($ws);
~~~

---

## 5 · Minimal WebSocket server with Fibers (≤ 5 channels)

**Why?**  
Prototype a low-traffic WebSocket endpoint—e.g. a *who-is-online* status
panel—without deploying Nginx Unit or Go. The Fiber budget caps active
chats at five, so RAM usage stays predictable.

~~~php
use Quicpro\Config;
use Quicpro\Server;
use Quicpro\Session;
use Fiber;

$cfg    = Config::new(['cert_file' => 'cert.pem', 'key_file' => 'key.pem']);
$server = new Server('::', 4433, $cfg);

$handle = function (Session $sess): void {
    static $open = 0;
    if ($open >= 5 || !$sess->isWebSocket('/chat')) { $sess->close(); return; }
    $ws = $sess->upgrade('/chat');
    $open++;
    while (!feof($ws)) fwrite($ws, strtoupper(fgets($ws)));
    fclose($ws); $sess->close(); $open--;
};

while ($c = $server->accept()) (new Fiber($handle))->start($c);
~~~

---

## 6 · Warm-start in FaaS / serverless containers

**Why?**  
Cut cold-start pain on platforms like AWS Lambda or Google Cloud Run: the
global `$GLOBAL_SESS` survives between invocations so repeat calls skip
both TCP and TLS handshakes.

~~~php
/* Global survives container reuse */
$GLOBAL_SESS = null;

function handler(array $event): string {
    global $GLOBAL_SESS;
    $GLOBAL_SESS ??= quicpro_connect('api.internal', 4433);

    $id = quicpro_send_request($GLOBAL_SESS, '/data?id=' . $event['id']);
    while (!$r = quicpro_receive_response($GLOBAL_SESS, $id)) {
        quicpro_poll($GLOBAL_SESS, 5);
    }
    return $r['body'];
}
~~~

---

## 7 · Collecting stats & QLOG traces

**Why?**  
Debug congestion control, retransmissions, or handshake details. QLOG is
the official QUIC trace format; tools like *qvis* and Wireshark read it.

~~~php
$s = quicpro_connect('example.org', 443);
quicpro_set_qlog_path($s, '/tmp/example.qlog');   // live trace
$id = quicpro_send_request($s, '/');
while (!quicpro_receive_response($s, $id)) quicpro_poll($s, 10);
print_r(quicpro_get_stats($s));
quicpro_close($s);
~~~
Visualise the file with qvis.dev or Wireshark.

---

## 8 · Zero-copy XDP path + busy-poll tuning

**Why?**  
Push latency to microsecond levels on dedicated hardware—think
high-frequency trading or telemetry bursts—by avoiding both syscalls and
kernel queues.

~~~php
putenv('QUICPRO_XDP=ens6');          // AF_XDP socket
putenv('QUICPRO_BUSY_POLL=50');      // spin 50 µs before Fiber yield

$s  = quicpro_connect('10.0.0.2', 4433);
$id = quicpro_send_request($s, '/ping');

while (!quicpro_receive_response($s, $id)) quicpro_poll($s, 0); // busy-poll

$lat = quicpro_get_stats($s)['rtt'];
printf("RTT: %.1f µs\n", $lat / 1000);
quicpro_close($s);
~~~
