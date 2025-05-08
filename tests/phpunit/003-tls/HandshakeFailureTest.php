<?php
declare(strict_types=1);

namespace QuicPro\Tests\TLS;

use PHPUnit\Framework\TestCase;
use Quicpro\Config;
use Quicpro\Session;

/*
 * ─────────────────────────────────────────────────────────────────────────────
 *  FILE: HandshakeFailureTest.php
 *  SUITE: 003-tls (handshake‑level failures)
 *
 *  WHY THIS TEST EXISTS
 *  --------------------
 *  A clean QUIC + TLS 1.3 handshake is the prerequisite for *every* higher
 *  protocol layer.  Equally important is to prove that **broken** handshakes
 *  are rejected in clearly defined ways so callers can implement graceful
 *  fall‑back or circuit‑breaker logic.  This test covers two normative
 *  failure scenarios:
 *
 *      3‑1  Self‑signed certificate ↛ peer verification = on   → TLS error
 *      3‑2  ALPN mismatch (client advertises hq‑29 only, server wants h3)
 *
 *  A compliant implementation MUST throw the documented exception classes
 *  so user‑land code can `catch (QuicTlsHandshakeException $e)` and decide
 *  what to do next (retry on TCP, show warning, etc.).
 *
 *  TEST ENVIRONMENT CONTRACT
 *  -------------------------
 *  • demo‑quic (listed in docker‑compose) runs a *self‑signed* certificate on
 *    UDP :4433.  The cert is intentionally **not** part of any system trust
 *    store, hence verify_peer = true MUST fail.
 *
 *      QUIC_DEMO_HOST   (default demo‑quic)
 *      QUIC_DEMO_PORT   (default 4433)
 *
 *  • The server negotiates ALPN = h3.  Advertising only hq‑29 therefore
 *    triggers a handshake alert.
 *
 *  IMPLEMENTATION NOTES
 *  --------------------
 *  We use the **OOP API** (Session object) instead of the procedural helper
 *  so we can pass a custom Config instance with tweaked options.  Both code
 *  paths map to the same C internals, hence coverage is identical.
 * ─────────────────────────────────────────────────────────────────────────────
 */
final class HandshakeFailureTest extends TestCase
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
     * 3‑1 – Self‑signed certificate MUST fail when verify_peer = true
     * ---------------------------------------------------------------
     * EXPECTATION:
     *     new Session() throws **QuicTlsHandshakeException** because the
     *     certificate cannot be validated against any trusted CA.
     */
    public function testSelfSignedCertificateFailsVerification(): void
    {
        $cfg = Config::new([
            'verify_peer' => true,              // enforce certificate check
            'alpn'        => ['h3'],            // correct ALPN, failure must be cert
        ]);

        $this->expectException(\QuicTlsHandshakeException::class);

        // Attempt the handshake – should explode before returning
        new Session($this->host, $this->port, $cfg); // intentionally unused
    }

    /*
     * 3‑2 – ALPN mismatch MUST abort the handshake
     * --------------------------------------------
     * EXPECTATION:
     *     Client advertises only **hq‑29** while server offers h3 ⇒ TLS alert
     *     mapped to **QuicTlsHandshakeException**.
     */
    public function testAlpnMismatchAbortsHandshake(): void
    {
        $cfg = Config::new([
            'verify_peer' => false,             // ignore cert this time
            'alpn'        => ['hq-29'],         // deliberately wrong
        ]);

        $this->expectException(\QuicTlsHandshakeException::class);

        new Session($this->host, $this->port, $cfg); // intentionally unused
    }
}
