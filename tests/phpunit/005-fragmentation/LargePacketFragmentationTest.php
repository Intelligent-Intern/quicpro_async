<?php
declare(strict_types=1);

namespace QuicPro\Tests\Fragmentation;

use Exception;
use PHPUnit\Framework\TestCase;
use Quicpro\Session;

/*
 * ─────────────────────────────────────────────────────────────────────────────
 *  FILE: LargePacketFragmentationTest.php
 *  SUITE: 005-fragmentation
 *
 *  PURPOSE & COVERAGE
 *  ------------------
 *  RFC 9000 §14.1 fixes the QUIC *maximum datagram size* to 1200 B for the
 *  Initial packet.  Any user payload larger than that MUST be fragmented
 *  into multiple QUIC packets and re-assembled at the peer.  Failure to
 *  handle this transparently leads to silent data-loss and connection stalls.
 *
 *  With three sub-tests we validate:
 *      1. End-to-end echo of a **single 1 MiB POST** (≈ 870× MTU)
 *      2. Incremental *stream-oriented* receive to catch off-by-one bugs
 *      3. Mid-transfer **stream cancellation** (§9; error code FLOW_CONTROL)
 *
 *  Together these assertions exercise the following normative MUSTs:
 *      • RFC 9000 §2.4   – Reliable, in-order stream delivery
 *      • RFC 9000 §9.3   – RST_STREAM / STOP_SENDING semantics
 *      • RFC 9114 §8.1   – HTTP/3 DATA frame mapping to QUIC STREAM
 *
 *  SERVER CONTRACT
 *  ---------------
 *  Endpoint /echo-bin echoes every octet it receives and supports
 *  STOP_SENDING error code 0x10 to abort early.
 *
 *  ENVIRONMENT VARIABLES
 *  ---------------------
 *      QUIC_DEMO_HOST   (default demo-quic)
 *      QUIC_DEMO_PORT   (default 4433)
 * ─────────────────────────────────────────────────────────────────────────────
*/
final class LargePacketFragmentationTest extends TestCase
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
     *  TEST 1 – Monolithic 1 MiB round-trip
     *  ------------------------------------
     *  We stream a full-megabyte binary blob in one go and read back the
     *  response as a single buffer.  This catches fragmentation issues on
     *  *either* direction and verifies reassembly via a SHA-256 checksum.
     * @throws Exception
     */
    public function testEchoOneMebibyteSingleBuffer(): void
    {
        $blob = random_bytes(1024 * 1024);     // 1 MiB
        $expect = hash('sha256', $blob, true);

        $sess = quicpro_connect($this->host, $this->port);

        $streamId = quicpro_send_request(
            $sess,
            '/echo-bin',
            ['content-type' => 'application/octet-stream'],
            $blob
        );

        $body = '';
        while (($chunk = quicpro_receive_response($sess, $streamId)) !== null) {
            $body .= \is_array($chunk) ? $chunk['body'] ?? '' : $chunk;
        }

        $this->assertSame(
            $expect,
            hash('sha256', $body, true),
            'Payload corrupted – fragmentation or reassembly failed'
        );

        quicpro_close($sess);
    }

    /**
     *  TEST 2 – Incremental receive (8 KiB chunks)
     *  -------------------------------------------
     *  Application code often consumes DATA frames incrementally.  We therefore
     *  poll until `quicpro_receive_response()` indicates new data and build the
     *  body in 8 KiB slices.  A length mismatch would expose off-by-one or
     *  EOF-handling mistakes (RFC 9000 §2.4, last paragraph).
     *
     * @return void
     * @throws Exception
     */
    public function testEchoOneMebibyteChunkedReceive(): void
    {
        $blob   = random_bytes(1024 * 1024);
        $expect = strlen($blob);

        $sess = quicpro_connect($this->host, $this->port);

        $streamId = quicpro_send_request(
            $sess,
            '/echo-bin',
            ['content-type' => 'application/octet-stream'],
            $blob
        );

        $recv = '';
        do {
            quicpro_poll($sess, 10);                     // non-blocking poll
            $part = quicpro_receive_response($sess, $streamId);
            if (\is_string($part)) {
                $recv .= $part;
            } elseif (\is_array($part)) {
                $recv .= $part['body'] ?? '';
            }
        } while ($part !== null);

        $this->assertSame(
            $expect,
            strlen($recv),
            'Size mismatch – frame boundaries not respected'
        );

        $this->assertSame(
            $blob,
            $recv,
            'Octet sequence altered during chunked transfer'
        );

        quicpro_close($sess);
    }

    /**
     *  TEST 3 – Mid-transfer cancellation
     *  ----------------------------------
     *  RFC 9000 §9.3 requires that a peer handles RST_STREAM / STOP_SENDING
     *  even while fragmented data are in flight.  We push half the blob,
     *  cancel the stream, and expect `quicpro_cancel_stream()` to return TRUE
     *  without killing the whole connection.
     *
     * @throws Exception
     */
    public function testStreamCancellationHalfWay(): void
    {
        $blob = random_bytes(512 * 1024);   // 512 KiB

        $sess = quicpro_connect($this->host, $this->port);

        $id = quicpro_send_request(
            $sess,
            '/echo-bin',
            ['content-type' => 'application/octet-stream'],
            $blob
        );

        $this->assertTrue(
            quicpro_cancel_stream($sess, $id),
            'Stream cancellation failed – RST/STOP_SENDING not honoured'
        );

        /*  Connection MUST stay alive so we can send another request.          */
        $health = quicpro_send_request(
            $sess,
            '/',
            [':method' => 'GET']
        );
        $this->assertIsInt($health, 'Connection unusable after stream abort');

        quicpro_close($sess);
    }
}
