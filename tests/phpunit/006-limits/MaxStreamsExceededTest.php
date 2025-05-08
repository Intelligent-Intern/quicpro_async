<?php
declare(strict_types=1);

namespace QuicPro\Tests\Limits;

use PHPUnit\Framework\TestCase;

/**
 * ─────────────────────────────────────────────────────────────────────────────
 *  FILE: MaxStreamsExceededTest.php
 *  SUITE: 006-limits
 *
 *  PURPOSE
 *  -------
 *  A single QUIC connection may carry only a bounded number of concurrent
 *  bidirectional streams.  This cap is negotiated during the handshake and
 *  raised by `MAX_STREAMS` frames (RFC 9000 §4.6).  The demo server is hard-
 *  wired to allow **64** streams at once; the sixty-fifth create attempt must
 *  fail with a `STREAM_LIMIT_ERROR`, which user-land surfaces as
 *  `QuicStreamException`.  Verifying that behaviour shields applications from
 *  silent resource exhaustion and detects broken flow-control accounting.
 *
 *  TEST STRATEGY
 *  -------------
 *      1. Open one connection that will be reused for all stream attempts.
 *      2. Fire 64 parallel GET requests against “/echo” – each MUST succeed.
 *      3. Attempt stream #65 – it MUST raise `QuicStreamException`.
 *      4. Cancel every open stream and close the session to restore state.
 *
 *  ENVIRONMENT
 *  -----------
 *      QUIC_DEMO_HOST   default demo-quic
 *      QUIC_DEMO_PORT   default 4433
 * ─────────────────────────────────────────────────────────────────────────────
 */
final class MaxStreamsExceededTest extends TestCase
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

    /**
     *  TEST – Stream-limit enforcement within a single session
     *  -------------------------------------------------------
     *  Sixty-four streams are expected to open; the sixty-fifth must throw.
     *  A finally-block guarantees teardown even if any assertion aborts early.
     */
    public function testSixtyFifthStreamIsRejected(): void
    {
        $sess      = quicpro_connect($this->host, $this->port);
        $streams   = [];

        try {
            /*  Create the permitted 64 concurrent streams first.  Each call to
             *  quicpro_send_request() returns a unique integer stream ID that
             *  remains valid until the application cancels or reads it to EOS.
             */
            for ($i = 0; $i < 64; $i++) {
                $id = quicpro_send_request($sess, '/echo');
                $this->assertIsInt($id, "Stream #$i did not return an ID");
                $streams[] = $id;
            }

            /*  The 65th request exceeds the peer’s advertised maximum.  RFC 9000
             *  dictates a STREAM_LIMIT_ERROR, which the extension maps to an
             *  exception so that callers can implement back-pressure or retry.
             */
            $this->expectException(\QuicStreamException::class);
            quicpro_send_request($sess, '/echo');
        } finally {
            /*  Cancel all streams to unblock transport-level flow control.     */
            foreach ($streams as $id) {
                quicpro_cancel_stream($sess, $id);
            }
            quicpro_close($sess);
        }
    }
}
