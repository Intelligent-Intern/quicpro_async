<?php
declare(strict_types=1);

namespace QuicPro\Tests\Resources;

use PHPUnit\Framework\TestCase;
use Quicpro\Session;

/*
 * ─────────────────────────────────────────────────────────────────────────────
 *  FILE: ConnectionCloseMemoryTest.php
 *  SUITE: 010-resources
 *
 *  WHY THIS TEST EXISTS
 *  --------------------
 *  Persistent memory growth after repeated *connect → close* cycles hints at
 *  descriptor leaks or orphaned buffers inside the PHP extension. Because such
 *  leaks surface only after hours or days in production, we simulate a burst of
 *  1 000 short-lived sessions and verify that the RSS delta stays within a tight
 *  margin (≤ 1 MiB). The threshold is deliberately stricter than production
 *  budgets to catch regressions early.
 *
 *  SPEC / BEST-PRACTICE REFERENCES
 *  --------------------------------
 *  • RFC 9000 §10.4 “Connection Migration” emphasizes clean state disposal.
 *  • ISO/IEC 9899  Any dynamic allocation requires a matching free — failing
 *    to do so constitutes undefined behaviour and may exhaust host resources.
 *
 *  TEST ENVIRONMENT CONTRACT
 *  -------------------------
 *  A lightweight QUIC echo server **mem-quic** runs inside the test network
 *  and delivers an immediate 200 OK + “pong” for GET /health. No TLS cost is
 *  incurred because session tickets are disabled; this keeps the inner loop
 *  CPU-bound on the extension rather than the crypto backend.
 *
 *      QUIC_MEM_HOST   (default mem-quic)
 *      QUIC_MEM_PORT   (default 4433)
 * ─────────────────────────────────────────────────────────────────────────────
 */
final class ConnectionCloseMemoryTest extends TestCase
{
    private const ITERATIONS   = 1000;   // number of connect-close cycles
    private const MEM_TOLERANCE = 1_048_576; // 1 MiB safety margin

    private string $host;
    private int    $port;

    protected function setUp(): void
    {
        if (!\function_exists('quicpro_version')) {
            self::markTestSkipped('quicpro_async extension not loaded');
        }

        $this->host = getenv('QUIC_MEM_HOST') ?: 'mem-quic';
        $this->port = (int) (getenv('QUIC_MEM_PORT') ?: 4433);

        /*  Flush stray cyclic references that earlier tests might have left
         *  behind so the initial memory snapshot is deterministic.           */
        gc_collect_cycles();
    }

    /*
     *  TEST – Repeated connect/close must not leak memory
     *  --------------------------------------------------
     *  1. Record baseline RSS.
     *  2. Run ITERATIONS handshake cycles, issuing a trivial /health request.
     *  3. Take a second RSS snapshot.
     *  4. Assert that the delta is below MEM_TOLERANCE bytes.
     *
     *  NOTE:
     *  rss() reads the resident-set size from /proc/self/statm on Linux and
     *  falls back to memory_get_usage(true) elsewhere. We avoid platform-
     *  specific extensions so the test remains portable across CI runners.
     */
    public function testNoMemoryLeakOnClose(): void
    {
        $baseline = $this->rss();

        for ($i = 0; $i < self::ITERATIONS; ++$i) {
            $sess = quicpro_connect($this->host, $this->port);
            $this->assertInstanceOf(Session::class, $sess, "Cycle $i failed connect");
            $this->assertTrue($sess->isConnected(), "Cycle $i handshake incomplete");

            $streamId = quicpro_send_request($sess, '/health');
            /*  Drain the minimal response to exercise stream plumbing.        */
            while (($chunk = quicpro_receive_response($sess, $streamId)) !== null) {/* ignore */}

            $sess->close();
            /*  Encourage timely release of Zend & libquic resources.          */
            unset($sess);
            gc_collect_cycles();
        }

        $after = $this->rss();
        $delta = $after - $baseline;

        $this->assertLessThan(
            self::MEM_TOLERANCE,
            $delta,
            sprintf('Resident set grew by %d bytes (> %d)', $delta, self::MEM_TOLERANCE)
        );
    }

    /**
     * Return the current resident-set size in bytes on POSIX systems or fall
     * back to PHP’s memory_get_usage(TRUE) which reports the allocator arena.
     */
    private function rss(): int
    {
        if (PHP_OS_FAMILY === 'Linux' && ($fh = @fopen('/proc/self/statm', 'r'))) {
            [$pages] = sscanf(fgets($fh), "%d");
            fclose($fh);

            return $pages * 4096; // system page size
        }

        return memory_get_usage(true);
    }
}
