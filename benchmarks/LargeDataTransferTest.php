<?php
declare(strict_types=1);

/*
 * bench_download_100mb.php
 * ─────────────────────────────────────────────────────────────────────────
 *  PURPOSE
 *  -------
 *  • Pure **throughput benchmark** – *not* a PHPUnit test – that measures
 *    end-to-end download speed of a 100 MiB object over an HTTP/3 data
 *    stream.  Results provide a rough upper bound for single-stream
 *    performance on the current network path and host workload.
 *
 *  • The script leaves integrity verification to unit tests and instead
 *    focuses on wall-clock time and bytes transferred, because that is
 *    the only metric that matters for raw bandwidth checks.
 *
 *  • Output is a single line on STDOUT so it can be parsed easily by
 *    CI dashboards or graphed by external tooling.
 * ─────────────────────────────────────────────────────────────────────────
 */

$host = getenv('QUIC_DEMO_HOST') ?: 'demo-quic';
$port = (int) (getenv('QUIC_DEMO_PORT') ?: 4433);

/*  Establish one QUIC session and request the fixed 100 MiB blob.          */
$sess      = quicpro_connect($host, $port);
$streamId  = quicpro_send_request($sess, '/blob/100mb');

$t0     = hrtime(true);            // high-resolution timer (ns)
$bytes  = 0;

while (true) {
    $chunk = quicpro_receive_response($sess, $streamId);

    /*  No progress yet – hand control back to the reactor so the engine
     *  can drive the network before we block again.                         */
    if ($chunk === null) {
        quicpro_poll($sess, 10);
        continue;
    }

    /*  Engine signalled stream completion with the final response array.
     *  Append last body part (if any) and break out of the loop.           */
    if (\is_array($chunk)) {
        $bytes += strlen($chunk['body']);
        break;
    }

    /*  Intermediate DATA chunk – accumulate byte counter.                  */
    $bytes += strlen($chunk);
}

$elapsedMs = (hrtime(true) - $t0) / 1_000_000.0;   // → milliseconds

printf(
    "Downloaded %.2f MiB in %.2f s → %.2f MiB/s\n",
    $bytes / 1_048_576,
    $elapsedMs / 1000.0,
    ($bytes / 1_048_576) / ($elapsedMs / 1000.0)
);

quicpro_close($sess);
