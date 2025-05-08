<?php
declare(strict_types=1);

namespace QuicPro\Tests\Connection;

use Exception;
use PHPUnit\Framework\TestCase;
use Quicpro\Session;

/*
 * ─────────────────────────────────────────────────────────────────────────────
 *  FILE: WebsocketTransferTest.php
 *  SUITE: 002-connection
 *
 *  WHY THIS TEST EXISTS
 *  --------------------
 *  *WebsocketOpenTest* proved we can **upgrade** to an echo WebSocket.
 *  Here we stress **data-plane correctness**:
 *
 *    1. Echo of a **binary payload ≥ 64 KiB** (crosses one QUIC packet)
 *    2. Echo of **three consecutive frames** in correct order
 *    3. Proper handling of **Ping → Pong** control frames
 *
 *  Together these cases validate:
 *      • RFC 6455 §5 (fragmentation & control frames)
 *      • RFC 9220 §6 (WebSocket frame mapping to H3 DATAGRAM/STREAM)
 *      • Internal flow control of quicpro_async’s ws wrapper
 *
 *  SERVER CONTRACT
 *  ---------------
 *  `demo-quic` endpoint **chat** echoes *each* received frame verbatim
 *  and replies to a Ping with the same Payload (standard behaviour).
 *
 *  ENVIRONMENT
 *  -----------
 *      QUIC_DEMO_HOST   default demo-quic
 *      QUIC_DEMO_PORT   default 4433
 * ─────────────────────────────────────────────────────────────────────────────
*/
final class WebsocketTransferTest extends TestCase
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
     *  TEST 1 – Echo of a 64 KiB binary frame
     *  --------------------------------------
     *  EXPECTATION:
     *      • fwrite() of $blob succeeds
     *      • fgets() (binary-safe) returns identical blob
     *
     *  RATIONALE:
     *      64 KiB exceeds a single QUIC frame in most setups; proves that
     *      quicpro_async reassembles fragmented WebSocket frames spread
     *      over several H3 DATA frames.
     */
    /**
     * @throws Exception
     */
    public function testLargeBinaryEcho(): void
    {
        $sess  = quicpro_connect($this->host, $this->port);
        $ws    = $sess->upgrade('/chat', binary: true);       // binary mode

        $blob  = random_bytes(64 * 1024);                    // 65 536 B
        fwrite($ws, $blob);
        $echo  = stream_get_contents($ws, strlen($blob));

        $this->assertSame($blob, $echo, 'Large binary echo mismatch');

        fclose($ws);
        $sess->close();
    }

    /*
     *  TEST 2 – Multiple frames arrive in order
     *  ----------------------------------------
     *  EXPECTATION:
     *      • Frames “one”, “two”, “three” echoed back in same order
     *
     *  RATIONALE:
     *      Detects buffering bugs where frames are coalesced or reordered
     *      inside the QUIC/H3 pipeline.
     */
    public function testOrderedFrameEcho(): void
    {
        $sess = quicpro_connect($this->host, $this->port);
        $ws   = $sess->upgrade('/chat');

        $frames = ["one\n", "two\n", "three\n"];
        foreach ($frames as $f) fwrite($ws, $f);

        $received = [fgets($ws), fgets($ws), fgets($ws)];
        $this->assertSame($frames, $received, 'Frame order corrupted');

        fclose($ws);
        $sess->close();
    }

    /*
     *  TEST 3 – Ping → Pong round-trip
     *  --------------------------------
     *  EXPECTATION:
     *      • quicpro_ws_ping() returns the Pong payload within 1 s
     *
     *  RATIONALE:
     *      Control frames are critical for keep-alives; RFC 6455 §5.5
     *      demands a Pong with identical application data.
     */
    public function testPingPong(): void
    {
        $sess = quicpro_connect($this->host, $this->port);
        $ws   = $sess->upgrade('/chat');

        $data = "PINGDATA";
        $this->assertTrue(quicpro_ws_ping($ws, $data), 'Ping send failed');

        $pong = quicpro_ws_wait_pong($ws, timeout_ms: 1000);
        $this->assertSame($data, $pong, 'Pong payload mismatch');

        fclose($ws);
        $sess->close();
    }
}
