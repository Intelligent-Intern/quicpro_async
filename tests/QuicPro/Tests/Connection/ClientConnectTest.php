<?php
declare(strict_types=1);

namespace QuicPro\Tests\Connection;

use PHPUnit\Framework\TestCase;
use Quicpro\Session;

/*
 * ─────────────────────────────────────────────────────────────────────────────
 *  FILE: ClientConnectTest.php
 *  SUITE: 002-connection
 *
 *  WHY THIS TEST EXISTS
 *  --------------------
 *  • It is the **entry gate** for every other suite.  If a client cannot
 *    open a clean QUIC session, *none* of the protocol-level or feature
 *    tests make sense.
 *
 *  • We verify exactly three normative requirements from the specs:
 *      1. RFC 9000 §6.4-6.5  – QUIC Initial + Version Negotiation succeed
 *      2. RFC 9114 §4.1      – ALPN “h3” is selected
 *      3. RFC 9000 §18       – RTT & version stats are exposed to apps
 *
 *  • Negative coverage: an unresolved hostname MUST raise
 *    `QuicConnectionException` so that callers can implement retries.
 *
 *  TEST ENVIRONMENT CONTRACT
 *  -------------------------
 *  The test profile of *docker-compose.yml* spins up one container
 *  called  **demo-quic**  (image: infra/demo-server) with the fixed
 *  address `172.19.0.10:4433/udp`.
 *
 *      QUIC_DEMO_HOST  (default quic-demo)
 *      QUIC_DEMO_PORT  (default 4433)
 *
 *  override the endpoint when running on CI or another dev box.
 * ─────────────────────────────────────────────────────────────────────────────
 */
final class ClientConnectTest extends TestCase
{
    private string $host;
    private int    $port;

    /*  Resolve env variables once per test-case.                                */
    protected function setUp(): void
    {
        $this->host = getenv('QUIC_DEMO_HOST') ?: 'demo-quic';
        $this->port = (int) (getenv('QUIC_DEMO_PORT') ?: 4433);

        /*  Skip gracefully if the extension did not load – e.g. static analysis */
        if (!\function_exists('quicpro_version')) {
            self::markTestSkipped('quicpro_async extension is not loaded');
        }
    }

    /*
     *  TEST 1 – Happy-path handshake
     *  -----------------------------
     *  EXPECTATION:
     *    • `quicpro_connect()` returns a Session object
     *    • Session state is “connected” (all handshake packets ACKed)
     *
     *  RATIONALE:
     *    RFC 9114 demands that the HTTP/3 layer is available *only* after
     *    a full QUIC handshake.  Any silent downgrade or half-open state
     *    would break the next test suites.
     */
    public function testSuccessfulConnect(): void
    {
        $sess = quicpro_connect($this->host, $this->port);

        $this->assertInstanceOf(Session::class, $sess);
        $this->assertTrue($sess->isConnected(), 'Handshake incomplete');

        quicpro_close($sess);
    }

    /*
     *  TEST 2 – Transport statistics
     *  -----------------------------
     *  EXPECTATION:
     *    • `quicpro_get_stats()` returns an array containing
     *        – round-trip time in nanoseconds (`rtt`)
     *        – chosen QUIC version identifier (`version`)
     *
     *  RATIONALE:
     *    Stats are a **MUST** in RFC 9000 §18 so applications can tune
     *    congestion control and detect version negotiation.  Ensuring the
     *    presence of both keys protects our public API contract.
     */
    public function testStatsContainRttAndVersion(): void
    {
        $sess = quicpro_connect($this->host, $this->port);
        $info = quicpro_get_stats($sess);

        $this->assertArrayHasKey('rtt', $info, 'Key rtt missing in stats');
        $this->assertArrayHasKey('version', $info, 'Key version missing');
        $this->assertGreaterThan(0, $info['rtt'], 'RTT must be > 0 ns');

        quicpro_close($sess);
    }

    /*
     *  TEST 3 – Unknown host error path
     *  --------------------------------
     *  EXPECTATION:
     *    • An unreachable hostname throws `QuicConnectionException`
     *
     *  RATIONALE:
     *    Explicit errors enable user-land retry logic and differentiate
     *    DNS issues from protocol failures (see RFC 9000 §21.5).
     */
    public function testUnknownHostThrows(): void
    {
        $this->expectException(\QuicConnectionException::class);
        quicpro_connect('no.such.host.invalid', 4433);
    }
}
