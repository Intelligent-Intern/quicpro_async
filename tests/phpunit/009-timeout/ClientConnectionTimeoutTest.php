<?php
declare(strict_types=1);

namespace QuicPro\Tests\Timeout;

use PHPUnit\Framework\TestCase;
use Quicpro\Session;

/**
 * ─────────────────────────────────────────────────────────────────────────────
 *  FILE: ClientConnectionTimeoutTest.php
 *  SUITE: 009-timeout
 *
 *  OVERVIEW
 *  --------
 *  This test suite ensures that the quicpro_async extension properly surfaces
 *  both handshake and idle timeouts as distinct exceptions. These exceptions
 *  allow applications to differentiate between a failed initial connection
 *  attempt and an established connection that becomes unresponsive.
 *
 *  SPEC REFERENCES
 *  ---------------
 *  1. QUIC Handshake Timeout - RFC 9000 §5.2
 *  2. QUIC Idle Timeout - RFC 9000 §10.1
 *
 *  TEST ENVIRONMENT
 *  ----------------
 *  - `blackhole-quic`: Simulates a server that never responds, causing a
 *    handshake timeout.
 *  - `sleepy-quic`: Simulates a server that becomes idle after the handshake,
 *    causing an idle timeout.
 *
 *  Both endpoints can be overridden by environment variables for CI or local
 *  testing.
 * ─────────────────────────────────────────────────────────────────────────────
 */
final class ClientConnectionTimeoutTest extends TestCase
{
    /** @var string Hostname for server that causes handshake timeout */
    private string $blackholeHost;

    /** @var int Port for blackhole server */
    private int $blackholePort;

    /** @var string Hostname for server that causes idle timeout */
    private string $sleepyHost;

    /** @var int Port for sleepy server */
    private int $sleepyPort;

    /**
     * Set up environment variables and check for required functions.
     *
     * The setup method ensures that the necessary environment variables are
     * set and that the quicpro_async extension is loaded before tests run.
     */
    protected function setUp(): void
    {
        if (!\function_exists('quicpro_version')) {
            self::markTestSkipped('quicpro_async extension not loaded');
        }

        $this->blackholeHost = getenv('QUIC_BLACKHOLE_HOST') ?: 'blackhole-quic';
        $this->blackholePort = (int)(getenv('QUIC_BLACKHOLE_PORT') ?: 4433);
        $this->sleepyHost    = getenv('QUIC_SLEEPY_HOST') ?: 'sleepy-quic';
        $this->sleepyPort    = (int)(getenv('QUIC_SLEEPY_PORT') ?: 4433);
    }

    /**
     * Test handshake timeout exception.
     *
     * Attempts to connect to a server that won't respond, expecting a
     * QuicHandshakeTimeoutException. The test also measures elapsed time to
     * ensure it doesn't exceed the handshake timeout threshold.
     */
    public function testHandshakeTimeoutException(): void
    {
        $t0 = microtime(true);

        try {
            quicpro_connect($this->blackholeHost, $this->blackholePort);
            $this->fail('Expected handshake timeout did not occur.');
        } catch (\QuicHandshakeTimeoutException $e) {
            $elapsed = (microtime(true) - $t0) * 1000; // Convert to ms
            $this->assertLessThan(2000, $elapsed,
                sprintf(
                    'Handshake timeout was expected within 2 seconds, but took %.2f ms.',
                    $elapsed
                )
            );
        }
    }

    /**
     * Test idle timeout exception.
     *
     * Connects to a server that becomes idle after handshake. Expects a
     * QuicIdleTimeoutException after the idle timeout period.
     */
    public function testIdleTimeoutException(): void
    {
        $sess = quicpro_connect($this->sleepyHost, $this->sleepyPort);
        $this->assertInstanceOf(Session::class, $sess);
        $this->assertTrue($sess->isConnected(), 'Session should be connected.');

        // Poll until idle timeout should occur
        $deadline = microtime(true) + 1.2; // 1.2 seconds to trigger idle timeout
        while (microtime(true) < $deadline) {
            quicpro_poll($sess, 50);
        }

        $this->expectException(\QuicIdleTimeoutException::class);
        quicpro_poll($sess, 0); // Should throw exception
    }
}
