<?php
require __DIR__ . '/vendor/autoload.php';

use function Amp\async;
use function Amp\Future\awaitAll;

array_shift($argv);                 // drop script name
$total = isset($argv[0]) ? (int)$argv[0] : 1000;
$conc  = isset($argv[1]) ? (int)$argv[1] : 100;

printf("QUIC/HTTP‑3  (%d req, %d conc)\n", $total, $conc);

/* ---------- quicpro_async ---------- */
$start   = hrtime(true);
$running = [];

for ($i = 0; $i < $total; $i++) {
    $running[] = async(function () {
        $s   = quicpro_connect('127.0.0.1', 4433, ['verify_peer' => false]);
        $sid = quicpro_send_request($s, '/');
        while (!$r = quicpro_receive_response($s, $sid)) {
            quicpro_poll($s, 10);
        }
        quicpro_close($s);
        return $r['body'];
    });

    if (count($running) === $conc) {
        awaitAll($running);
        $running = [];
    }
}
awaitAll($running);
$elapsed = (hrtime(true) - $start) / 1e9;
printf("QUIC  : %.1f req/s  (%.3f s)\n", $total / $elapsed, $elapsed);

/* ---------- cURL / HTTP 1.1 -------- */
printf("HTTP‑1.1 via libcurl (sequential, same #req)\n");

$curlStart = hrtime(true);
$ch = curl_init();
curl_setopt_array($ch, [
    CURLOPT_URL            => 'http://127.0.0.1:8000/',  // <‑ hier TCP‑Server angeben
    CURLOPT_RETURNTRANSFER => true,
    CURLOPT_FOLLOWLOCATION => true,
]);

for ($i = 0; $i < $total; $i++) {
    curl_exec($ch);              // blocking
}
curl_close($ch);

$curlElapsed = (hrtime(true) - $curlStart) / 1e9;
printf("cURL  : %.1f req/s  (%.3f s)\n", $total / $curlElapsed, $curlElapsed);