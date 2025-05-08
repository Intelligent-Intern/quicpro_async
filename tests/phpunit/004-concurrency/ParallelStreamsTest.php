<?php
declare(strict_types=1);

namespace QuicPro\Tests\Stream;

use PHPUnit\Framework\TestCase;
use Quicpro\Session;
use Quicpro\Stream;

/*
 * ─────────────────────────────────────────────────────────────────────────────
 *  FILE: ParallelStreamsTest.php
 *  SUITE: 005-stream
 *
 *  WHY THIS TEST EXISTS
 *  --------------------
 *  • HTTP/3 relies on QUIC’s *independent* stream abstraction for true
 *    multiplexing without head-of-line blocking (RFC 9114 §2).
 *  • An implementation that mishandles parallel streams defeats the main
 *    objective of QUIC and breaks higher-layer guarantees.
 *
 *  COVERED REQUIREMENTS
 *  --------------------
 *      1. Client MAY open many streams concurrently (RFC 9000 §9.2).
 *      2. Library MUST respect MAX_STREAMS limits negotiated with the peer
 *         and raise `QuicStreamException` when exceeded (RFC 9000 §4.6).
 *      3. Stream IDs on the client side MUST increase by a fixed increment
 *         of 4 and never collide (RFC 9000 §2.1).
 *
 *  TEST ENVIRONMENT CONTRACT
 *  -------------------------
 *  The demo server echoes back any bytes it receives on a stream.
 *
 *      QUIC_DEMO_HOST  (default demo-quic)
 *      QUIC_DEMO_PORT  (default 4433)
 * ─────────────────────────────────────────────────────────────────────────────
 */
final class ParallelStreamsTest extends TestCase
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
     *  TEST 1 – 100 bidirectional streams in parallel
     *  ----------------------------------------------
     *  EXPECTATION:
     *      • Every stream reaches “open” state.
     *      • Echo payload is received intact.
     */
    public function testHundredParallelStreams(): void
    {
        $sess     = quicpro_connect($this->host, $this->port);
        $streams  = [];
        $payload  = random_bytes(32);          // opaque data, avoids compression

        /* Open → write → FIN for 100 streams                                     */
        for ($i = 0; $i < 100; ++$i) {
            $st = $sess->openStream(true);     // bidirectional
            $this->assertInstanceOf(Stream::class, $st, "Stream $i failed");

            $written = $st->write($payload);
            $this->assertSame(strlen($payload), $written, "Write len $i mismatch");

            $st->shutdown();                  // client-FIN, keep receive open
            $streams[] = $st;
        }

        /* Read echoes in arbitrary order – independence is the point             */
        foreach ($streams as $idx => $st) {
            $echo = $st->readAll();            // helper wraps read loop
            $this->assertSame(
                $payload,
                $echo,
                "Echo payload mismatch on stream $idx"
            );
            $st->close();
        }

        quicpro_close($sess);
    }

    /*
     *  TEST 2 – Stream ID monotonicity & spacing
     *  -----------------------------------------
     *  EXPECTATION:
     *      • IDs are strictly increasing by 4 (client-initiated bidirectional
     *        streams use the least-significant two bits “00”).
     */
    public function testStreamIdIncrementsByFour(): void
    {
        $sess = quicpro_connect($this->host, $this->port);

        $first = $sess->openStream(true)->getId();
        $second = $sess->openStream(true)->getId();
        $third = $sess->openStream(true)->getId();

        $this->assertSame(
            0,
            ($first   & 0b11),
            'Client-bidirectional stream ID must end in 00'
        );
        $this->assertSame($first + 4, $second, 'ID increment #1 wrong');
        $this->assertSame($second + 4, $third, 'ID increment #2 wrong');

        quicpro_close($sess);
    }

    /*
     *  TEST 3 – Respect MAX_STREAMS limit
     *  ----------------------------------
     *  EXPECTATION:
     *      • Exceeding the peer-advertised limit throws QuicStreamException.
     *
     *  NOTE:
     *      • `$limit` can be tuned to the value configured in the demo server.
     */
    public function testExceedingMaxStreamsThrows(): void
    {
        $sess  = quicpro_connect($this->host, $this->port);
        $limit = 64;                               // demo-server limit

        try {
            for ($i = 0; $i <= $limit; ++$i) {
                $sess->openStream(true);
            }
            $this->fail('Expected QuicStreamException was not thrown');
        } catch (\QuicStreamException $e) {
            $this->assertStringContainsString(
                'MAX_STREAMS',
                $e->getMessage()
            );
        } finally {
            quicpro_close($sess);
        }
    }
}
