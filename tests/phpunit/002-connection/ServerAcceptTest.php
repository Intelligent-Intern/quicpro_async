<?php
declare(strict_types=1);

namespace QuicPro\Tests\Connection;

use PHPUnit\Framework\TestCase;
use Quicpro\Server;
use Quicpro\Session;

/*
 * ─────────────────────────────────────────────────────────────────────────────
 *  FILE: ServerAcceptTest.php
 *  SUITE: 002-connection
 *
 *  WHY THIS TEST EXISTS
 *  --------------------
 *  • Complements *ClientConnectTest*: we now validate the **server-side**
 *    API of the extension by binding a UDP socket, accepting one client
 *    and echoing a trivial HTTP/3 response.
 *  • Covers RFC 9000 §6 (listener creation) and §10 (connection ID
 *    routing) indirectly—if `accept()` returns a Session object the
 *    handshake logic, CID parsing and STREAM frames all worked.
 *
 *  TEST STRATEGY
 *  -------------
 *  1. Spin up an in-process QUIC server on `127.0.0.1:$PORT` in a child
 *     process (pcntl_fork).  `$PORT` is a random high number > 50 000
 *     so no root privileges are required.
 *  2. In the parent, connect with `quicpro_connect()` and wait for the
 *     first request to be echoed back.
 *  3. Cleanly shut down both sides and assert that:
 *        • Server->accept() yielded a Session
 *        • Client saw HTTP/3 status 200
 *
 *  ENVIRONMENT REQUIREMENTS
 *  ------------------------
 *  • pcntl extension must be loaded (all test images enable it).
 *  • No additional docker-compose services are necessary because the
 *    server lives inside the test process → **shell scripts unchanged**.
 * ─────────────────────────────────────────────────────────────────────────────
 */
final class ServerAcceptTest extends TestCase
{
    public function testServerAcceptsOneConnection(): void
    {
        if (!extension_loaded('pcntl')) {
            self::markTestSkipped('pcntl required for fork');
        }

        $port = random_int(50000, 60000);
        $pid  = pcntl_fork();

        if ($pid === -1) {
            self::fail('pcntl_fork failed');
        }

        /* ── CHILD: run minimal QUIC echo server ─────────────────────────── */
        if ($pid === 0) {
            $cfg = \Quicpro\Config::new([
                'cert_file' => '/workspace/tests/certs/server.pem',
                'key_file'  => '/workspace/tests/certs/server.pem',
                'alpn'      => ['h3'],
            ]);

            $srv = new Server('127.0.0.1', $port, $cfg);
            /** @var Session $sess */
            $sess = $srv->accept(5000); // wait max 5 s

            if (!$sess) {
                exit(1);               // parent will notice via waitpid
            }

            /* echo fixed 200 OK response then close                           */
            $req    = $sess->receiveRequest();
            $stream = $req->stream();
            $stream->respond(200, ['content-type' => 'text/plain']);
            $stream->sendBody("hi\n", fin: true);
            $sess->close();
            exit(0);
        }

        /* ── PARENT: act as client ───────────────────────────────────────── */
        $sess    = quicpro_connect('127.0.0.1', $port);
        $stream  = quicpro_send_request($sess, '/');
        while (!$resp = quicpro_receive_response($sess, $stream)) {
            quicpro_poll($sess, 20);
        }
        quicpro_close($sess);

        /* check HTTP status code == 200                                      */
        $this->assertSame(200, $resp['headers'][':status'] ?? null);

        /* make sure child exited cleanly                                     */
        pcntl_waitpid($pid, $status);
        $this->assertSame(0, pcntl_wexitstatus($status), 'server failed');
    }
}
