<?php
declare(strict_types=1);

namespace QuicPro\Tests\Limits;

use PHPUnit\Framework\TestCase;
use Quicpro\Session;

/*
 * ─────────────────────────────────────────────────────────────────────────────
 *  FILE: MaxConnectionsExceededTest.php
 *  SUITE: 006-limits
 *
 *  PURPOSE
 *  -------
 *  A production-grade QUIC stack MUST throttle excessive connection attempts
 *  in order to mitigate resource exhaustion attacks (cf. RFC 9000 §21.4;
 *  “Denial-of-Service”).  The demo server running inside `demo-quic` is
 *  configured with a **hard limit of 8 concurrent sessions**.  The ninth
 *  attempt shall be rejected at the transport layer and surface in user land
 *  as `QuicConnectionException`.
 *
 *  TEST STRATEGY
 *  -------------
 *      1. Create eight *healthy* sessions – they MUST succeed.
 *      2. Attempt a **ninth** connection – it MUST throw.
 *      3. Clean up all handles to restore the global state.
 *
 *  ENVIRONMENT
 *  -----------
 *      QUIC_DEMO_HOST   default demo-quic
 *      QUIC_DEMO_PORT   default 4433
 * ─────────────────────────────────────────────────────────────────────────────
 */
final class MaxConnectionsExceededTest extends TestCase
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
     *  TEST – Connection limit enforcement
     *  -----------------------------------
     *  8 sessions must connect; the 9th must raise QuicConnectionException.
     *  Every handle is closed in a finally-block to avoid leaking resources.
     */
    public function testNinthConnectionIsRejected(): void
    {
        $handles = [];

        try {
            /*  First eight sessions are expected to open without error.       */
            for ($i = 0; $i < 8; $i++) {
                $sess = quicpro_connect($this->host, $this->port);
                $this->assertInstanceOf(Session::class, $sess, "Session #$i failed");
                $handles[] = $sess;
            }

            /*  A ninth attempt MUST exceed the server’s cap and throw.        */
            $this->expectException(\QuicConnectionException::class);
            quicpro_connect($this->host, $this->port);
        } finally {
            /*  Graceful shutdown of all sessions even if an assertion failed. */
            foreach ($handles as $h) {
                if ($h instanceof Session) {
                    $h->close();
                } elseif (\is_resource($h)) {
                    quicpro_close($h);
                }
            }
        }
    }
}
