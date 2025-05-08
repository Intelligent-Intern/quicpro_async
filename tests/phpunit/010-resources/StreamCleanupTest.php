<?php
declare(strict_types=1);

namespace QuicPro\Tests\Resources;

use PHPUnit\Framework\TestCase;
use Quicpro\Session;

/*
 * ─────────────────────────────────────────────────────────────────────────────
 *  FILE: StreamCleanupTest.php
 *  SUITE: 010-resources
 *
 *  WHY THIS TEST EXISTS
 *  --------------------
 *  Every unidirectional or bidirectional stream that reaches a terminal state
 *  (Reset, Finished, or Cancelled) MUST be expunged from all in-memory tracking
 *  tables. Otherwise the maximum of 2^62-1 open-stream identifiers defined in
 *  RFC 9000 §2.1 eventually becomes an *effective* limit in long-running
 *  processes, leading to exhaustion or performance collapse.
 *
 *  This test therefore opens **512 sequential request streams**, cancels each
 *  immediately, and asserts that the resident-set size (RSS) stays within a
 *  1 MiB safety margin. The value is intentionally tight so that even a single
 *  lingering stream map entry will trip the alarm during CI.
 *
 *  SPEC / BEST-PRACTICE REFERENCES
 *  --------------------------------
 *  • RFC 9000 §3 “Streams and Flow Control” — states & finalisation rules
 *  • RFC 9114 §6 “HTTP Request Lifecycle” — client cancel equals QUIC RESET_STREAM
 *  • CERT MEM35-C — “Allocate and free memory dynamically” (defensive coding)
 *
 *  TEST ENVIRONMENT CONTRACT
 *  -------------------------
 *  A local test server **mem-quic** listens on UDP 4433 and responds “204 No
 *  Content” to any request. The body size is zero to keep the inner loop light.
 *
 *      QUIC_MEM_HOST   (default mem-quic)
 *      QUIC_MEM_PORT   (default 4433)
 * ─────────────────────────────────────────────────────────────────────────────
 */
final class StreamCleanupTest extends TestCase
{
    private const STREAMS        = 512;          // open/cancel iterations
    private const MEM_TOLERANCE  = 1_048_576;    // ≤ 1 MiB allowed growth

    private string $host;
    private int    $port;

    protected function setUp(): void
    {
        if (!\function_exists('quicpro_version')) {
            self::markTestSkipped('quicpro_async extension not loaded');
        }

        $this->host = getenv('QUIC_MEM_HOST') ?: 'mem-quic';
        $this->port = (int) (getenv('QUIC_MEM_PORT') ?: 4433);

        /*  Ensure prior suites do not skew the baseline measurement.          */
        gc_collect_cycles();
    }

    /*
     *  TEST – Cancelling many streams must not leak memory
     *  ---------------------------------------------------
     *  1. Capture baseline RSS.
     *  2. Create STREAMS request streams and cancel each one.
     *  3. Flush engine event-loop via quicpro_poll().
     *  4. Close the QUIC session and free userland refs.
     *  5. Compare RSS again and assert delta < MEM_TOLERANCE.
     */
    public function testStreamStateFreedAfterCancel(): void
    {
        $baseline = $this->rss();

        $sess = quicpro_connect($this->host, $this->port);
        $this->assertInstanceOf(Session::class, $sess);
        $this->assertTrue($sess->isConnected(), 'Initial handshake failed');

        for ($i = 0; $i < self::STREAMS; ++$i) {
            $id = quicpro_send_request($sess, '/abort');
            $this->assertIsInt($id, "Stream $i creation failed");
            $this->assertTrue(
                quicpro_cancel_stream($sess, $id),
                "Stream $i cancel returned false"
            );
        }

        /*  Give the engine a moment to propagate RESET_STREAM / STOP_SENDING
         *  and retire flow-control credits.                                   */
        while (quicpro_poll($sess, 10)) {/* drain events */}
        $sess->close();
        unset($sess);

        gc_collect_cycles();                 // final GC sweep
        $delta = $this->rss() - $baseline;

        $this->assertLessThan(
            self::MEM_TOLERANCE,
            $delta,
            sprintf('RSS grew by %d bytes (> %d)', $delta, self::MEM_TOLERANCE)
        );
    }

    /** Return current resident-set size in bytes (Linux) or allocator usage. */
    private function rss(): int
    {
        if (PHP_OS_FAMILY === 'Linux' && ($fh = @fopen('/proc/self/statm', 'r'))) {
            [$pages] = sscanf(fgets($fh), "%d");
            fclose($fh);
            return $pages * 4096;            // page size
        }

        return memory_get_usage(true);
    }
}
