# quicpro_async – Usage Examples

Below you find a handful of micro‑snippets that show how to use the
extension in different scenarios.  

---

## 1 · Single GET request

~~~php
<?php
$sess  = quicpro_connect('example.org', 443);
$stream = quicpro_send_request($sess, '/');

while (!$resp = quicpro_receive_response($sess, $stream)) {
    quicpro_poll($sess, 50);
}

echo $resp['body'];
quicpro_close($sess);
~~~

---

## 2 · Parallel requests with Fibers (PHP 8.1+)

~~~php
<?php
function fetch(string $host, string $path): string {
    $sess  = quicpro_connect($host, 443);
    $id    = quicpro_send_request($sess, $path);

    while (!$r = quicpro_receive_response($sess, $id)) {
        quicpro_poll($sess, 10);   // yields automatically
    }
    quicpro_close($sess);
    return $r['body'];
}

$fibers = [];
foreach (['/a', '/b', '/c'] as $p) {
    $fibers[] = new Fiber(fn() => fetch('example.org', $p));
}

foreach ($fibers as $f) $f->start();
foreach ($fibers as $f) echo $f->getReturn(), PHP_EOL;
~~~

---

## 3 · Object‑oriented wrapper

~~~php
<?php
class Client {
    private $sess;
    public function __construct(string $host, int $port = 443) {
        $this->sess = quicpro_connect($host, $port);
    }
    public function get(string $path): string {
        $id = quicpro_send_request($this->sess, $path);
        while (!$r = quicpro_receive_response($this->sess, $id)) {
            quicpro_poll($this->sess, 10);
        }
        return $r['body'];
    }
    public function __destruct() { quicpro_close($this->sess); }
}

$cli = new Client('example.org');
echo $cli->get('/foo');
~~~

---

## 4 · Warm‑start in FaaS / Serverless

~~~php
<?php
// Re‑use one global session across invocations (if container stays warm)
global $qp_conn;

function handler(array $event) {
    global $qp_conn;
    if (!$qp_conn) {
        $qp_conn = quicpro_connect('api.internal', 4433);
    }

    $id = quicpro_send_request($qp_conn, '/data');
    while (!$r = quicpro_receive_response($qp_conn, $id)) {
        quicpro_poll($qp_conn, 10);
    }
    return $r['body'];
}
~~~

---

## 5 · Collect connection stats

~~~php
<?php
$s = quicpro_connect('example.org', 443);
$id = quicpro_send_request($s, '/');
while (!quicpro_receive_response($s, $id)) {
    quicpro_poll($s, 10);
}
$stats = quicpro_get_stats($s);
print_r($stats);
quicpro_close($s);
~~~

---