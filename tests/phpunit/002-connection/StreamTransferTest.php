<?php
declare(strict_types=1);

namespace QuicPro\Tests\Connection;

use PHPUnit\Framework\TestCase;
use Quicpro\Stream;

/*
 * ─────────────────────────────────────────────────────────────────────────────
 *  FILE: StreamTransferTest.php
 *  SUITE: 002-connection
 *
 *  WHY THIS TEST EXISTS
 *  --------------------
 *  After *StreamOpenTest* proved we can **create** streams, this file
 *  exercises actual **payload transfer** in both directions.  We cover
 *  three normative behaviours:
 *
 *    1. Correct echo of a **small payload**   – RFC 9114 §6 “DATA frames”
 *    2. Correct echo of a **large (>1 MiB) payload** that spans several
 *       QUIC packets – RFC 9000 §4 “Packetization” + flow control
 *    3. Proper handling of **FIN / half-close** semantics so the peer
 *       can still send data after we close our write side – RFC 9000 §3.2
 *
 *  TEST SERVER CONTRACT
 *  --------------------
 *   `demo-quic` exposes an endpoint **echo** that replies with the
 *    request body verbatim (status 200).  For large uploads the server
 *    streams the response as it receives data so we can verify popping
 *    multiple read events in PHP.
 *
 *  ENVIRONMENT
 *  -----------
 *      QUIC_DEMO_HOST   default demo-quic
 *      QUIC_DEMO_PORT   default 4433
 *  No extra Docker services or scripts are required.
 * ─────────────────────────────────────────────────────────────────────────────
*/
final class StreamTransferTest extends TestCase
{
    private string $host;
    private int    $port;

    protected function setUp(): void
    {
        $this->host = getenv('QUIC_DEMO_HOST') ?: 'demo-quic';
        $this->port = (int) (getenv('QUIC_DEMO_PORT') ?: 4433);

        if (!function_exists('quicpro_version')) {
            self::markTestSkipped('quicpro_async extension not loaded');
        }
    }

    /*
     *  TEST 1 – Small payload round-trip (5 bytes)
     *  -------------------------------------------
     *  EXPECTATION:
     *      Response body === request body (“ping\n”)
     *
     *  RATIONALE:
     *      Baseline DATA frame encode/parse must work before we attempt
     *      bigger transfers.  Failure here is most likely header/stream
     *      mis-ordering.
     */
    public function testEchoSmallPayload(): void
    {
        $sess   = quicpro_connect($this->host, $this->port);
        $body   = "ping\n";
        $sid    = quicpro_send_request($sess, '/echo',
            ['content-type'=>'text/plain'], $body);

        while (!$resp = quicpro_receive_response($sess, $sid)) {
            quicpro_poll($sess, 10);
        }
        $this->assertSame($body, $resp['body']);

        quicpro_close($sess);
    }

    /*
     *  TEST 2 – Large payload (1 MiB) across multiple packets
     *  ------------------------------------------------------
     *  EXPECTATION:
     *      • Response length == 1 048 576 bytes
     *      • Content identical to upload
     *
     *  RATIONALE:
     *      Stresses flow-control windows and fragment reassembly in the
     *      C layer.  Detects off-by-one in offset calculation and ensures
     *      quicpro_send_request() can stream bodies > MAX_DATAGRAM_SIZE.
     */
    public function testEchoLargePayload(): void
    {
        $sess = quicpro_connect($this->host, $this->port);
        $body = random_bytes(1024 * 1024);               // 1 MiB
        $sid  = quicpro_send_request($sess, '/echo',
            ['content-type'=>'application/octet-stream'], $body);

        while (!$resp = quicpro_receive_response($sess, $sid)) {
            quicpro_poll($sess, 10);
        }
        $this->assertSame(strlen($body), strlen($resp['body']), 'Size mismatch');
        $this->assertSame($body, $resp['body'], 'Payload corrupted');

        quicpro_close($sess);
    }

    /*
     *  TEST 3 – FIN handling (half-close)
     *  ----------------------------------
     *  EXPECTATION:
     *      • `quicpro_finish_body()` returns true
     *      • After FIN the client can still **receive** server data
     *
     *  RATIONALE:
     *      HTTP/3 mandates that endpoints may close their send direction
     *      while continuing to read (full-duplex).  This test guarantees
     *      that the wrapper honours FIN flags and that the server side
     *      sees END_STREAM early.
     */
    public function testFinishBodyAndReceiveAfterFin(): void
    {
        $sess = quicpro_connect($this->host, $this->port);
        $sid  = quicpro_send_request($sess, '/echo', [], '');

        // Immediately finish the request body (zero-length)
        $this->assertTrue(quicpro_finish_body($sess, $sid), 'FIN failed');

        // Server responds with "done\n"
        while (!$resp = quicpro_receive_response($sess, $sid)) {
            quicpro_poll($sess, 10);
        }
        $this->assertSame("done\n", $resp['body']);

        quicpro_close($sess);
    }
}
