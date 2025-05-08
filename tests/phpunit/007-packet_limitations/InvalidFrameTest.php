<?php
declare(strict_types=1);

namespace QuicPro\Tests\PacketLimitations;

use PHPUnit\Framework\TestCase;

/**
 * ─────────────────────────────────────────────────────────────────────────────
 *  FILE: InvalidFrameTest.php
 *  SUITE: 007-packet_limitations
 *
 *  MOTIVATION
 *  ----------
 *  HTTP/3 restricts the maximum size of a compressed header block to
 *  prevent amplification attacks and resource exhaustion.  RFC 9114 §4.3
 *  stipulates that a peer MUST terminate the connection with
 *  `H3_MESSAGE_ERROR` (mapped by *quicpro_async* to `QuicFrameException`)
 *  if it receives an excessively large HEADERS frame.
 *
 *  The demo server limits **compressed** header blocks to 16 KiB.  Sending
 *  a single oversized custom header therefore provokes the mandatory error
 *  path and allows us to verify the extension’s exception mapping.
 *
 *  TEST PLAN
 *  ---------
 *    1. Establish a single QUIC session to the demo endpoint.
 *    2. Craft a request to “/echo” with header **x-oversized** containing
 *       65 535 bytes (five times the permitted 16 KiB).
 *    3. `quicpro_send_request()` is expected to throw `QuicFrameException`
 *       immediately once the frame is enqueued.
 *    4. Always close the session in a finally-block to keep CI state clean.
 *
 *  ENVIRONMENT
 *  -----------
 *      QUIC_DEMO_HOST   default demo-quic
 *      QUIC_DEMO_PORT   default 4433
 * ─────────────────────────────────────────────────────────────────────────────
 */
final class InvalidFrameTest extends TestCase
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
     *  TEST – Oversized HEADERS frame triggers QuicFrameException
     *  ----------------------------------------------------------
     *  The client constructs an 64 KiB custom header, surpassing the demo
     *  server’s 16 KiB limit.  RFC-compliant behaviour is to abort the
     *  connection with `H3_MESSAGE_ERROR`, which the PHP binding converts
     *  to `QuicFrameException`.  A try/finally guard ensures teardown.
     */
    public function testOversizedHeaderBlockIsRejected(): void
    {
        $sess = quicpro_connect($this->host, $this->port);

        try {
            $hugeHeader = str_repeat('A', 65_535);   // 64 KiB + 31 B

            $this->expectException(\QuicFrameException::class);
            quicpro_send_request(
                $sess,
                '/echo',
                headers: ['x-oversized' => $hugeHeader]
            );
        } finally {
            quicpro_close($sess);
        }
    }
}
