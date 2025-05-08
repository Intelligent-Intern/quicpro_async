<?php
declare(strict_types=1);

namespace QuicPro\Tests\Retry;

use PHPUnit\Framework\TestCase;
use Quicpro\Session;

/*
 * ─────────────────────────────────────────────────────────────────────────────
 *  FILE: ClientReconnectTest.php
 *  SUITE: 008-retry
 *
 *  WHY THIS TEST EXISTS
 *  --------------------
 *  • QUIC’s **stateless retry** mechanism (RFC 9000 §8) is the first line of
 *    defence against amplification attacks.  A compliant client MUST
 *    recognise the Retry packet, validate the integrity tag, cache the token,
 *    and automatically re-send Initial + 0-RTT to the *new* connection ID.
 *
 *  • We verify three normative requirements:
 *      1. Successful handshake **after** one Retry round-trip.
 *      2. Presence of a “retry” flag inside transport statistics (public API).
 *      3. A second connection attempt that re-uses the now-invalid token
 *         MUST raise `QuicConnectionException` (RFC 9000 §17.2.5).
 *
 *  TEST ENVIRONMENT CONTRACT
 *  -------------------------
 *  docker-compose profile “tests” launches **demo-quic** with
 *      QUIC_RETRY_ONCE = true         → first Initial triggers a Retry
 *      QUIC_RETRY_TOKEN_LIFETIME = 1  → single-use token
 *
 *      QUIC_DEMO_HOST   default demo-quic
 *      QUIC_DEMO_PORT   default 4433
 * ─────────────────────────────────────────────────────────────────────────────
 */
final class ClientReconnectTest extends TestCase
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
     *  TEST 1 – Handshake succeeds after Retry
     *  ---------------------------------------
     *  EXPECTATION:
     *      • quicpro_connect() returns a connected Session.
     *      • Transport stats expose "retry" => true.
     *
     *  RATIONALE:
     *      Confirms automatic handling of Retry packets and keeps our public
     *      metrics contract intact for congestion control algorithms.
     */
    public function testRetryHandshakeSucceeds(): void
    {
        $sess = quicpro_connect($this->host, $this->port);

        $this->assertInstanceOf(Session::class, $sess);
        $this->assertTrue($sess->isConnected(), 'Handshake incomplete');

        $stats = quicpro_get_stats($sess);
        $this->assertArrayHasKey('retry', $stats, 'Key retry missing in stats');
        $this->assertTrue($stats['retry'], 'Retry flag must be true');

        quicpro_close($sess);
    }

    /*
     *  TEST 2 – Re-use of consumed Retry token fails
     *  ---------------------------------------------
     *  EXPECTATION:
     *      • A *second* connect() replays the expired token and MUST throw
     *        QuicConnectionException (server responds with INVALID_TOKEN).
     *
     *  RATIONALE:
     *      Ensures the engine does **not** silently ignore INVALID_TOKEN and
     *      that callers receive a deterministic error for proper retry logic.
     */
    public function testExpiredTokenThrows(): void
    {
        /*  First connection consumes the single-use token                       */
        $first = quicpro_connect($this->host, $this->port);
        quicpro_close($first);

        /*  Second attempt should fail                                           */
        $this->expectException(\QuicConnectionException::class);
        quicpro_connect($this->host, $this->port);
    }

    /*
     *  TEST 3 – Stats reflect zero Retry on direct path
     *  -----------------------------------------------
     *  EXPECTATION:
     *      • When the server is configured to *skip* Retry (env override),
     *        the stats array MUST expose "retry" => false.
     *
     *  RATIONALE:
     *      Validates that the flag is accurate and not hard-wired to true.
     *      A false positive would break telemetry dashboards.
     */
    public function testStatsWithoutRetry(): void
    {
        /*  A dedicated host alias “demo-quic-noretry” bypasses the Retry filter.
         *  CI pipelines set up both endpoints so we can flip behaviour at will. */
        $host = getenv('QUIC_NORETRY_HOST') ?: 'demo-quic-noretry';

        $sess  = quicpro_connect($host, $this->port);
        $stats = quicpro_get_stats($sess);

        $this->assertArrayHasKey('retry', $stats);
        $this->assertFalse($stats['retry'], 'Retry flag should be false');

        quicpro_close($sess);
    }
}
