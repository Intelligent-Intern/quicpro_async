<?php
declare(strict_types=1);

namespace QuicPro\Tests\Connection;

use PHPUnit\Framework\TestCase;
use Quicpro\Session;

/*
 * ─────────────────────────────────────────────────────────────────────────────
 *  FILE: HandshakeTest.php
 *  SUITE: 002-connection
 *
 *  WHY THIS TEST EXISTS
 *  --------------------
 *  Whereas *ClientConnectTest* checks “can we connect at all?”, this file
 *  digs into the **fine details** of a *successful* QUIC/TLS handshake:
 *
 *      • ALPN negotiation: h3 MUST be chosen              (RFC 9114 §4.1)
 *      • Retry support: server-initiated Retry MUST work  (RFC 9000 §8)
 *      • 0-RTT safety: app data MUST be blocked until the
 *        handshake is confirmed                           (RFC 9000 §4.1)
 *
 *  A failure here indicates a bug in the C wrapper around *quiche*
 *  rather than in userland PHP.
 *
 *  TEST ENV CONTRACT
 *  -----------------
 *  The docker profile “test” launches **demo-quic** on
 *  `172.19.0.10:4433/udp`.  A *second* port 4434 deliberately issues a
 *  Retry packet so we can exercise that path.
 *
 *      QUIC_DEMO_HOST      default: demo-quic
 *      QUIC_DEMO_PORT      default: 4433   (no Retry)
 *      QUIC_RETRY_PORT     default: 4434   (always Retry)
 * ─────────────────────────────────────────────────────────────────────────────
 */
final class HandshakeTest extends TestCase
{
    private string $host;
    private int    $port;
    private int    $retryPort;

    protected function setUp(): void
    {
        $this->host      = getenv('QUIC_DEMO_HOST')  ?: 'demo-quic';
        $this->port      = (int) (getenv('QUIC_DEMO_PORT')  ?: 4433);
        $this->retryPort = (int) (getenv('QUIC_RETRY_PORT') ?: 4434);

        if (!function_exists('quicpro_version')) {
            self::markTestSkipped('quicpro_async extension not loaded');
        }
    }

    /*
     *  TEST 1 – ALPN MUST be “h3”
     *  --------------------------
     *  EXPECTATION:
     *      quicpro_get_alpn($sess) returns exactly **h3**
     *
     *  RATIONALE:
     *      Applications that depend on HTTP/3 functionality (push,
     *      WebTransport, …) must confirm ALPN before using the socket.
     *      A mismatch would silently fall back to an earlier draft.
     */
    public function testNegotiatesH3Alpn(): void
    {
        $sess = quicpro_connect($this->host, $this->port);

        $alpn = quicpro_get_alpn($sess);      // wrapper helper
        $this->assertSame('h3', $alpn, 'ALPN not h3');

        quicpro_close($sess);
    }

    /*
     *  TEST 2 – Client handles server Retry
     *  ------------------------------------
     *  EXPECTATION:
     *      • connect to $retryPort succeeds
     *      • stats show `retry = true`
     *
     *  RATIONALE:
     *      Some deployments (DoS mitigation) force a Retry.  The client
     *      must resend the Initial with the token and **must not**
     *      expose the dance to PHP (RFC 9000 §8).  A failure breaks
     *      real-world CDNs like Cloudflare or Fastly.
     */
    public function testServerRetryPacketHandled(): void
    {
        $sess = quicpro_connect($this->host, $this->retryPort);
        $info = quicpro_get_stats($sess);

        $this->assertNotEmpty($info['retry'] ?? null, 'Retry flag not set');

        quicpro_close($sess);
    }

    /*
     *  TEST 3 – No application data before handshake confirmation
     *  ----------------------------------------------------------
     *  EXPECTATION:
     *      quicpro_send_request() with 0-length body MUST block until
     *      the handshake is confirmed; early send returns false.
     *
     *  RATIONALE:
     *      RFC 9000 §4.1 forbids app data on unconfirmed 0-RTT streams
     *      (replay risk).  The binding should surface that via a simple
     *      return-value guard so userland can wait or retry.
     */
    public function testNoDataSentBeforeHandshakeConfirmed(): void
    {
        $sess = quicpro_connect($this->host, $this->port);

        $stream = quicpro_send_request($sess, '/probe', ['accept'=>'*/*'], '');
        $this->assertFalse($stream, 'Data sent before handshake complete');

        // Handshake finishes after one poll iteration (max 20 ms here)
        while (!quicpro_receive_response($sess, $stream)) {
            quicpro_poll($sess, 20);
        }
        quicpro_close($sess);
    }
}
