<?php
declare(strict_types=1);

namespace QuicPro\Tests\Tls;

use PHPUnit\Framework\TestCase;
use Quicpro\Config;
use Quicpro\Session;

/*
 * ─────────────────────────────────────────────────────────────────────────────
 *  FILE: InvalidCertificateTest.php
 *  SUITE: 003-tls
 *
 *  WHY THIS TEST EXISTS
 *  --------------------
 *  • RFC 8446 §4.4.2 mandates certificate validation during the TLS 1.3
 *    handshake.  When validation fails, the QUIC connection MUST abort.
 *  • An application that enables `verify_peer` relies on the library to
 *    reject untrusted certificates deterministically.
 *
 *  NEGATIVE COVERAGE
 *  -----------------
 *  • If the peer presents a self-signed or otherwise untrusted certificate,
 *    `QuicTlsHandshakeException` MUST be thrown so that callers can decide
 *    whether to retry with a different trust anchor.
 *
 *  TEST ENVIRONMENT CONTRACT
 *  -------------------------
 *      QUIC_DEMO_HOST  (default demo-quic)
 *      QUIC_DEMO_PORT  (default 4433)
 *
 *  The demo server inside docker-compose presents a self-signed leaf cert
 *  that is *not* in the system trust store.
 * ─────────────────────────────────────────────────────────────────────────────
 */
final class InvalidCertificateTest extends TestCase
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
     *  TEST – Handshake aborts on untrusted cert
     *  -----------------------------------------
     *  EXPECTATION:
     *      • Session construction throws QuicTlsHandshakeException.
     */
    public function testHandshakeFailsWithUntrustedCert(): void
    {
        $cfg = Config::new([
            'verify_peer' => true,
        ]);

        $this->expectException(\QuicTlsHandshakeException::class);

        new Session($this->host, $this->port, $cfg);
    }
}
