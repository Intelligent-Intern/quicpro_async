<?php
declare(strict_types=1);

namespace QuicPro\Tests\PacketLimitations;

use PHPUnit\Framework\TestCase;
use Quicpro\Session;

/*
 * ─────────────────────────────────────────────────────────────────────────────
 *  FILE: MalformedPacketTest.php
 *  SUITE: 007-packet_limitations
 *
 *  WHY THIS TEST EXISTS
 *  --------------------
 *  RFC 9000 defines an unambiguous wire grammar for every QUIC packet
 *  (§17).  A single out-of-spec octet MUST trigger an immediate
 *  CONNECTION_CLOSE with error code PROTOCOL_VIOLATION or
 *  FRAME_ENCODING_ERROR (§10.2).  To validate that the client reacts
 *  correctly, the demo server offers two routes that *deliberately*
 *  transmit malformed packets:
 *
 *      • /bad-length      – Length field < actual payload
 *      • /reserved-bits   – Short-header packet with reserved bits = 0
 *
 *  EXPECTATIONS
 *  ------------
 *      1. quicpro_async aborts the connection and throws
 *         `QuicProtocolViolationException`.
 *      2. `quicpro_get_last_error()` returns a non-empty diagnostic string.
 *
 *  ENVIRONMENT
 *  -----------
 *      QUIC_DEMO_HOST   default demo-quic
 *      QUIC_DEMO_PORT   default 4433
 * ─────────────────────────────────────────────────────────────────────────────
 */
final class MalformedPacketTest extends TestCase
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
     *  TEST 1 – Length field mismatch (RFC 9000 §17.2)
     *  ----------------------------------------------
     *  The server sends a long-header packet whose Length is *smaller* than
     *  the real payload.  A compliant client MUST consider this fatal.
     */
    public function testLengthFieldMismatchTriggersAbort(): void
    {
        $this->expectException(\QuicProtocolViolationException::class);

        $sess = quicpro_connect($this->host, $this->port);
        quicpro_send_request($sess, '/bad-length');   // corruption happens server-side

        while (quicpro_poll($sess, 50)) {/* spin until exception */}
    }

    /*
     *  TEST 2 – Reserved bits cleared (RFC 9000 §17.3)
     *  -----------------------------------------------
     *  Both reserved bits in a short-header packet MUST be set to 1.  The
     *  demo endpoint flips them to 0 – the client has to abort.
     */
    public function testReservedBitsViolation(): void
    {
        $this->expectException(\QuicProtocolViolationException::class);

        $sess = quicpro_connect($this->host, $this->port);
        quicpro_send_request($sess, '/reserved-bits');

        while (quicpro_poll($sess, 50)) {/* spin until exception */}
    }

    /*
     *  TEST 3 – Diagnostic string populated
     *  ------------------------------------
     *  After a protocol violation the human-readable error buffer MUST not
     *  be empty – otherwise debugging CI failures becomes guesswork.
     */
    public function testLastErrorNonEmptyAfterViolation(): void
    {
        try {
            $sess = quicpro_connect($this->host, $this->port);
            quicpro_send_request($sess, '/bad-length');

            while (quicpro_poll($sess, 50)) {/* spin */}
        } catch (\QuicProtocolViolationException) {
            $this->assertNotSame('', quicpro_get_last_error(), 'Diagnostic string empty');
            return;
        }

        $this->fail('Protocol violation did not raise any exception');
    }
}
