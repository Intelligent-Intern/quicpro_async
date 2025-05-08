<?php
declare(strict_types=1);

namespace QuicPro\Tests\Timeout;

use PHPUnit\Framework\TestCase;
use Quicpro\Session;
use QuicResponseTimeoutException;
use function function_exists;

/**
 * ─────────────────────────────────────────────────────────────────────────────
 *  FILE: ServerResponseTimeoutTest.php
 *  SUITE: 009-timeout
 *
 *  PURPOSE
 *  -------
 *  Ensure that the client tears down a *single HTTP/3 stream* when a server
 *  fails to deliver *any* response data within a reasonable period, while the
 *  underlying QUIC connection itself remains healthy. This guards against
 *  resource-leaks caused by endpoints that accept requests but never reply.
 *
 *  SPEC REFERENCES
 *  ---------------
 *  • RFC 9114 §6.5  –  Clients are expected to abandon a request if the peer
 *    is unable or unwilling to send a response within a sensible time-frame.
 *  • RFC 9000 §10.2 –  Endpoints SHOULD signal application errors promptly
 *    so that transport resources can be reclaimed.
 *
 *  TEST SCENARIO
 *  -------------
 *  A helper container **slow-quic** completes the handshake but purposefully
 *  blocks forever on the /never path. The extension exposes a per-stream
 *  timer (configured to 1 s in php.ini) that must raise
 *  `QuicResponseTimeoutException` once expired.
 *
 *      QUIC_SLOW_HOST   (default slow-quic)
 *      QUIC_SLOW_PORT   (default 4433)
 * ─────────────────────────────────────────────────────────────────────────────
*/
final class ServerResponseTimeoutTest extends TestCase
{
    private string $host;
    private int    $port;

    protected function setUp(): void
    {
        if (!function_exists('quicpro_version')) {
            self::markTestSkipped('quicpro_async extension not loaded');
        }

        $this->host = getenv('QUIC_SLOW_HOST') ?: 'slow-quic';
        $this->port = (int) (getenv('QUIC_SLOW_PORT') ?: 4433);
    }

    /*
     *  TEST – Response-header timeout
     *  ------------------------------
     *  1. Open a QUIC session.
     *  2. Issue GET /never which never produces headers.
     *  3. Spin the event-loop long enough for the 1 s stream timer to fire.
     *  EXPECT:
     *      • Stream-level QuicResponseTimeoutException is thrown.
     *      • Connection remains established, enabling follow-up requests.
     */
    public function testResponseTimeoutException(): void
    {
        $sess = quicpro_connect($this->host, $this->port);
        $this->assertInstanceOf(Session::class, $sess);
        $this->assertTrue($sess->isConnected(), 'Handshake must succeed');

        $streamId = quicpro_send_request($sess, '/never');

        /* Allow the internal timer to expire (1 s + safety margin). */
        $deadline = microtime(true) + 1.1;
        while (microtime(true) < $deadline) {
            quicpro_poll($sess, 50);              // 20 Hz spin-loop
        }

        $this->expectException(QuicResponseTimeoutException::class);
        quicpro_receive_response($sess, $streamId);

        $this->assertTrue($sess->isConnected(), 'Connection should survive');
        $sess->close();
    }
}
