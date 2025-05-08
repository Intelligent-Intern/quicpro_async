<?php
declare(strict_types=1);

namespace QuicPro\Tests\Retry;

use PHPUnit\Framework\TestCase;
use Quicpro\Session;

/**
 * ─────────────────────────────────────────────────────────────────────────────
 *  FILE: StreamRetryTest.php
 *  SUITE: 008-retry
 *
 *  WHY THIS TEST EXISTS
 *  --------------------
 *  •  QUIC permits a peer to abort a single stream with RESET_STREAM /
 *     STOP_SENDING while keeping the connection alive.  When the error code
 *     maps to HTTP/3 0x010C (H3_REQUEST_CANCELLED) **and** the outstanding
 *     request is idempotent, a compliant client SHOULD transparently open a
 *     *new* stream and retransmit the request once.              ― RFC 9114 §9
 *
 *  •  We validate three normative behaviours of quicpro_async:
 *        1.  A GET to /unstable that is cancelled once by the server
 *            is automatically retried and ends in a 200 response.
 *        2.  Transport statistics expose `"stream_retry" => 1`.
 *        3.  A non-idempotent POST **must not** be retried; the engine
 *            raises `QuicStreamException` so that application code decides.
 *
 *  SERVER CONTRACT
 *  ---------------
 *  The container **demo-quic** serves two helper endpoints:
 *
 *      /unstable      – Cancels the first incoming stream with H3_REQUEST_CANCELLED
 *                       but succeeds on the immediate retry.
 *      /post-unstable – Always cancels the first stream so the library
 *                       would have to violate spec to succeed.
 *
 *  ENVIRONMENT
 *  -----------
 *      QUIC_DEMO_HOST   default demo-quic
 *      QUIC_DEMO_PORT   default 4433
 * ─────────────────────────────────────────────────────────────────────────────
 */
final class StreamRetryTest extends TestCase
{
    private string $host;
    private int    $port;

    protected function setUp(): void
    {
        $this->host = getenv('QUIC_DEMO_HOST') ?: 'demo-quic';
        $this->port = (int) (getenv('QUIC_DEMO_PORT') ?: 4433);

        if (!\function_exists('quicpro_version')) {
            self::markTestSkipped('quicpro_async extension not loaded');
        }
    }

    /*
     *  TEST 1 – Automatic retry of idempotent request
     *  ---------------------------------------------
     *  EXPECTATION:
     *      • final response carries   :status => 200
     *      • stats array contains     "stream_retry" => 1
     */
    public function testIdempotentRetrySucceeds(): void
    {
        $sess   = quicpro_connect($this->host, $this->port);
        $id     = quicpro_send_request($sess, '/unstable');

        /*  Spin the event-loop until a full response object is available       */
        $resp   = null;
        while ($resp === null) {
            $resp = quicpro_receive_response($sess, $id);
            /*  Flush engine timers / network                                  */
            while (quicpro_poll($sess, 50)) {/* spin */}
        }

        $this->assertIsArray($resp);
        $this->assertSame(200, $resp[':status'], 'Automatic retry failed');

        $stats = quicpro_get_stats($sess);
        $this->assertArrayHasKey('stream_retry', $stats);
        $this->assertSame(1, $stats['stream_retry'], 'Exactly one retry expected');

        quicpro_close($sess);
    }

    /*
     *  TEST 2 – No automatic retry for unsafe method
     *  --------------------------------------------
     *  EXPECTATION:
     *      • First stream is cancelled by the server.
     *      • Library surfaces `QuicStreamException` to the caller.
     *      • No hidden second attempt is made.
     */
    public function testNonIdempotentPostRaises(): void
    {
        $sess = quicpro_connect($this->host, $this->port);

        $this->expectException(\QuicStreamException::class);
        quicpro_send_request(
            $sess,
            '/post-unstable',
            headers: [':method' => 'POST', 'content-type' => 'text/plain'],
            body: 'SIDE-EFFECTS'
        );

        /*  If the engine tried to mask the error and silently retried, the
         *  exception would not bubble up and the assertion above would fail. */

        quicpro_close($sess);
    }
}
