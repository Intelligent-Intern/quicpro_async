<?php
declare(strict_types=1);

namespace QuicPro\Tests\Connection;

use PHPUnit\Framework\TestCase;
use Quicpro\Session;

/*
 * ─────────────────────────────────────────────────────────────────────────────
 *  FILE: MultipleConnectionsTest.php
 *  SUITE: 004-connection
 *
 *  WHY THIS TEST EXISTS
 *  --------------------
 *  • Many real-world applications maintain several concurrent QUIC sessions
 *    (e.g. HTTP/3 connection pools, DNS-over-QUIC, WebTransport).
 *  • RFC 9000 §5 requires that each connection be isolated in terms of
 *    packet number space and congestion control.  The library must therefore
 *    support multiple *independent* sessions without interference.
 *
 *  COVERED REQUIREMENTS
 *  --------------------
 *      1. A client MAY open multiple connections to the same endpoint
 *         (RFC 9000 §9.3).
 *      2. A connection limit reached MUST abort with QuicConnectionException
 *         (library-defined, maps to RFC 9000 §21.4 CONNECTION_ERROR).
 *
 *  TEST ENVIRONMENT CONTRACT
 *  -------------------------
 *      QUIC_DEMO_HOST  (default demo-quic)
 *      QUIC_DEMO_PORT  (default 4433)
 * ─────────────────────────────────────────────────────────────────────────────
 */
final class MultipleConnectionsTest extends TestCase
{
    private string $host;
    private int    $port;

    protected function setUp(): void
    {
        $this->host = getenv('QUIC_DEMO_HOST') ?: 'demo-quic';
        $this->port = (int)(getenv('QUIC_DEMO_PORT') ?: 4433);

        if (!\function_exists('quicpro_version')) {
            self::markTestSkipped('quicpro_async extension is not loaded');
        }
    }

    /*
     *  TEST 1 – Ten parallel sessions succeed
     *  --------------------------------------
     *  EXPECTATION:
     *      • All sessions reach “connected” state independently.
     */
    public function testTenParallelConnections(): void
    {
        $sessions = [];

        for ($i = 0; $i < 10; ++$i) {
            $sess = quicpro_connect($this->host, $this->port);
            $this->assertInstanceOf(Session::class, $sess, "Index $i failed");
            $this->assertTrue($sess->isConnected(), "Handshake $i incomplete");
            $sessions[] = $sess;
        }

        foreach ($sessions as $sess) {
            quicpro_close($sess);
        }
    }

    /*
     *  TEST 2 – Library enforces connection limit
     *  ------------------------------------------
     *  EXPECTATION:
     *      • When the limit is exceeded, QuicConnectionException is thrown.
     *
     *  NOTE:
     *      • Adjust `$limit` to the soft maximum configured in quicpro.
     */
    public function testExceedingConnectionLimitThrows(): void
    {
        $limit     = 128;
        $sessions  = [];

        try {
            for ($i = 0; $i < $limit; ++$i) {
                $sessions[] = quicpro_connect($this->host, $this->port);
            }
        } catch (\QuicConnectionException $e) {
            /* Clean up whatever succeeded before assertion */
            foreach ($sessions as $sess) {
                quicpro_close($sess);
            }
            $this->expectException(\QuicConnectionException::class);
            throw $e;   // re-throw for PHPUnit
        }

        /*  If we reached here, the limit was *not* enforced – fail hard.        */
        foreach ($sessions as $sess) {
            quicpro_close($sess);
        }
        $this->fail('Expected QuicConnectionException was not thrown');
    }
}
