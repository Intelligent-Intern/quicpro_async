# quicpro_async – Usage Examples

Below are **eight pragmatic examples** – from the most basic *Hello‑World* to a fully fledged HTTP/3 WebSocket server – that demonstrate how to use the quicpro_async extension from PHP 8.1 ‑ 8.4.

---

## 1 · Single `GET` Request (synchronous)
```php
$sess   = quicpro_connect('example.org', 443);
$stream = quicpro_send_request($sess, '/');

while (!$resp = quicpro_receive_response($sess, $stream)) {
    quicpro_poll($sess, 20);   // blocks ≤ 20 ms per loop
}

echo $resp['body'];
quicpro_close($sess);
```

---

## 2 · Parallel Requests with Fibers (concurrent client)
```php
function fetch(string $path): string {
    $s = quicpro_connect('example.org', 443);
    $id = quicpro_send_request($s, $path);
    while (!$r = quicpro_receive_response($s, $id)) {
        quicpro_poll($s, 5);    // yields inside Fiber
    }
    quicpro_close($s);
    return $r['body'];
}

$jobs = ['/a', '/b', '/c'];
$fibers = array_map(fn($p) => new Fiber(fn() => fetch($p)), $jobs);
array_walk($fibers, fn($f) => $f->start());
foreach ($fibers as $f) echo $f->getReturn(), PHP_EOL;
```

---

## 3 · Streaming `POST` Upload (chunked body)
```php
$s   = quicpro_connect('uploader.local', 8443);
$sid = quicpro_send_request($s, '/upload', headers: ['content-type' => 'application/octet-stream']);

$file = fopen('dump.raw', 'rb');
while (!feof($file)) {
    quiche_h3_send_body_chunk($s, $sid, fread($file, 16 * 1024));
    quicpro_poll($s, 0);                  // non‑blocking
}
quiche_h3_finish_body($s, $sid);

while (!$r = quicpro_receive_response($s, $sid)) {
    quicpro_poll($s, 10);
}
print_r($r);
```

---

## 4 · WebSocket Chat *Client* over HTTP/3
```php
$sess  = quicpro_connect('chat.example', 443);
$ws    = $sess->upgrade('/chat');         // RFC 9220 CONNECT → WebSocket

fwrite($ws, "hello from PHP\n");
while (!feof($ws)) {
    echo "Server: ", fgets($ws), PHP_EOL;
}
```

---

## 5 · Minimal WebSocket *Server* with Fibers (≤ 5 channels)
```php
use Quicpro\{Config, Server, Session};
$cfg    = Config::new([ 'cert_file' => 'cert.pem', 'key_file' => 'key.pem' ]);
$server = new Server('::', 4433, $cfg);

$handle = function(Session $sess): void {
    static $open = 0;
    if ($open >= 5 || !$sess->isWebSocket('/chat')) { $sess->close(); return; }
    $ws = $sess->upgrade('/chat');
    $open++;
    while (!feof($ws)) fwrite($ws, strtoupper(fgets($ws)));
    fclose($ws); $sess->close(); $open--; };

while (true) if ($c = $server->accept()) (new Fiber($handle))->start($c);
```

---

## 6 · Warm‑Start in FaaS / Serverless Containers
```php
// Global handle survives AWS Lambda / Google Cloud Run cold‑start boundaries
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
```

---

## 7 · Collecting Stats & Writing to QLOG
```php
$s = quicpro_connect('example.org', 443);
quicpro_set_qlog_path($s, '/tmp/example.qlog');   // enable live trace
$id = quicpro_send_request($s, '/');
while (!quicpro_receive_response($s, $id)) quicpro_poll($s, 10);
print_r(quicpro_get_stats($s));
quicpro_close($s);
```
The file `/tmp/example.qlog` can be visualised with **qvis** or Wireshark.

---

## 8 · Zero‑Copy XDP Path plus Busy‑Poll Tuning (extreme‑low‑latency)
```php
putenv('QUICPRO_XDP=ens6');          // selects AF_XDP socket for interface
putenv('QUICPRO_BUSY_POLL=50');      // spin 50 µs before fiber suspend

$s = quicpro_connect('10.0.0.2', 4433);
$id = quicpro_send_request($s, '/ping');

while (!quicpro_receive_response($s, $id)) {
    quicpro_poll($s, 0);            // 0 = busy‑poll only
}

$lat = quicpro_get_stats($s)['rtt'];
printf("RTT: %.1f µs\n", $lat / 1000);
quicpro_close($s);
```
