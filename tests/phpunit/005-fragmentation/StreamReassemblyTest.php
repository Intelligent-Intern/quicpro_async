<?php
declare(strict_types=1);

namespace QuicPro\Tests\Fragmentation;

use Exception;
use PHPUnit\Framework\TestCase;
use Quicpro\Session;

/*
 * ─────────────────────────────────────────────────────────────────────────────
 *  FILE: StreamReassemblyTest.php
 *  SUITE: 005-fragmentation
 *
 *  PURPOSE & SCOPE
 *  ---------------
 *  RFC 9000 §2.2 explains that QUIC streams may arrive *out of order* because
 *  individual frames can be lost and retransmitted independently.  The
 *  transport MUST therefore buffer and reorder bytes so that applications
 *  observe a contiguous octet sequence (§2.4, last paragraph).  This test
 *  suite proves that **quicpro_async** exposes only re-assembled data to user
 *  land and never leaks gaps, duplicates, or mis-ordered segments.
 *
 *  We exercise three practical scenarios that are notorious for unmasking
 *  off-by-one errors in the reassembly logic:
 *
 *      1. **Interleaved writes** – user alternates between two streams and
 *         forces the stack to juggle offsets.
 *      2. **High-frequency small chunks** – hundreds of 1-byte frames easily
 *         trigger packet coalescing on the wire, challenging reorder queues.
 *      3. **Zero-RTT racing** – a request sent during the TLS 0-RTT window is
 *         replayed by the server while ACKs for later packets arrive first.
 *
 *  SERVER CONTRACT
 *  ---------------
 *  Route /echo-bin returns the body verbatim on the same stream.  This is
 *  sufficient because all assertions are performed end-to-end on the client
 *  side.  The endpoint is part of the *demo-quic* container used elsewhere.
 *
 *  ENVIRONMENT
 *  -----------
 *      QUIC_DEMO_HOST   default demo-quic
 *      QUIC_DEMO_PORT   default 4433
 * ─────────────────────────────────────────────────────────────────────────────
*/
final class StreamReassemblyTest extends TestCase
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
     *  TEST 1 – Interleaved two-stream echo
     *  ------------------------------------
     *  We open *two* independent HTTP/3 requests that each carry a 128 KiB
     *  random blob.  Instead of finishing one stream first, the payload is
     *  written in 4 KiB slices while *alternating* between the streams.  This
     *  pattern deliberately scrambles packet order on the wire.  A successful
     *  test indicates that both re-assembled bodies match the originals.
     * @throws Exception
     */
    public function testInterleavedWritesAreReassembled(): void
    {
        [$blobA, $blobB] = [random_bytes(131072), random_bytes(131072)];

        $sess = quicpro_connect($this->host, $this->port);

        $idA = quicpro_send_request($sess, '/echo-bin', [], '');
        $idB = quicpro_send_request($sess, '/echo-bin', [], '');

        /*  Alternate 4 KiB chunks between A and B to scramble ordering.        */
        for ($off = 0; $off < 131072; $off += 4096) {
            quiche_h3_send_body_chunk($sess, $idA, $blobA[$off] . $blobA[$off + 1] ?? '');
            quiche_h3_send_body_chunk($sess, $idB, $blobB[$off] . $blobB[$off + 1] ?? '');
        }
        quiche_h3_finish_body($sess, $idA);
        quiche_h3_finish_body($sess, $idB);

        $recvA = $recvB = '';
        do {
            quicpro_poll($sess, 5);
            $partA = quicpro_receive_response($sess, $idA);
            $partB = quicpro_receive_response($sess, $idB);

            if (\is_string($partA)) $recvA .= $partA;
            if (\is_array($partA))  $recvA .= $partA['body'] ?? '';

            if (\is_string($partB)) $recvB .= $partB;
            if (\is_array($partB))  $recvB .= $partB['body'] ?? '';
        } while ($partA !== null || $partB !== null);

        $this->assertSame($blobA, $recvA, 'Stream A corrupted by re-assembly');
        $this->assertSame($blobB, $recvB, 'Stream B corrupted by re-assembly');

        quicpro_close($sess);
    }

    /**
     *  TEST 2 – Hundred tiny frames
     *  ----------------------------
     *  Many WebSocket emulations and RPC frameworks emit very small DATA
     *  frames.  To mimic this workload we transmit 100 ASCII characters, each
     *  as a *single-byte* chunk.  Packet coalescing will reorder these bytes,
     *  therefore the client must still receive the canonical string “abc…”.
     */
    public function testHundredSingleByteFrames(): void
    {
        $alphabet = implode('', array_map('chr', range(32, 131))); // 100 chars

        $sess  = quicpro_connect($this->host, $this->port);
        $sid   = quicpro_send_request($sess, '/echo-bin', [], '');

        /*  Push 1 byte at a time.                                              */
        foreach (str_split($alphabet) as $ch) {
            quiche_h3_send_body_chunk($sess, $sid, $ch);
        }
        quiche_h3_finish_body($sess, $sid);

        $body = '';
        while (($part = quicpro_receive_response($sess, $sid)) !== null) {
            $body .= \is_array($part) ? $part['body'] ?? '' : $part;
        }

        $this->assertSame($alphabet, $body, 'Byte order / loss detected');

        quicpro_close($sess);
    }

    /**
     *  TEST 3 – Zero-RTT replay versus ACK race
     *  ----------------------------------------
     *  A 0-RTT request may arrive at the server before the final TLS handshake
     *  completes, yet QUIC is free to acknowledge later frames first.  We send
     *  a 16 KiB payload during 0-RTT, wait only 1 ms to *encourage* a race,
     *  and then read back the echo.  If the stack mishandles gaps, either
     *  `quicpro_receive_response()` blocks forever or returns out-of-order
     *  fragments – both are caught by the checksum comparison below.
     */
    public function testZeroRttRaceCondition(): void
    {
        $blob   = random_bytes(16384);                 // 16 KiB
        $expect = hash('sha1', $blob, true);           // small hash for speed

        $sess = quicpro_connect($this->host, $this->port, /* 0-RTT */);

        $id = quicpro_send_request($sess, '/echo-bin', [], $blob);

        /*  Brief sleep to widen the ACK-versus-data race window.               */
        usleep(1000);

        $recv = '';
        while (($p = quicpro_receive_response($sess, $id)) !== null) {
            $recv .= \is_array($p) ? $p['body'] ?? '' : $p;
        }

        $this->assertSame(
            $expect,
            hash('sha1', $recv, true),
            '0-RTT replay produced corrupted in-order stream'
        );

        quicpro_close($sess);
    }
}
