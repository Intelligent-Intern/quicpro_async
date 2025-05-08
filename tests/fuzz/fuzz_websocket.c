/*
 * ─────────────────────────────────────────────────────────────────────────────
 *  FILE: fuzz_websocket.c
 *  TARGET: libFuzzer  (clang ≥ 11,  -fsanitize=fuzzer,address,undefined)
 *
 *  PURPOSE
 *  -------
 *  This harness stress-tests **WebSocket frame processing inside QUIC / H3**
 *  by exercising the bidirectional data-plane after a successful protocol
 *  upgrade (RFC 6455 §4, RFC 9220 §6).  It keeps one persistent QUIC session
 *  alive across all fuzz iterations so that per-connection state like
 *  FIN/RSV bits, masking keys, sequence ordering and flow-control windows
 *  accumulate naturally – a prerequisite for uncovering state-dependent
 *  memory corruption or logic flaws.
 *
 *  INPUT MODEL
 *  -----------
 *      byte 0        →  flags          (bit-field, see below)
 *      byte 1        →  opcode         (RFC 6455 §5.2)
 *      bytes 2-3     →  virtual Δt     (uint16 LE, micro-seconds)
 *      bytes 4-†     →  payload bytes  (unrestricted size)
 *
 *      flags bit 0   = FIN            (if clear → fragmented message)
 *      flags bit 1   = enable Ping    (send Ping *before* payload)
 *      flags bit 2   = close after Tx (simulate FIN/Close trigger)
 *
 *  BUILD
 *  -----
 *      clang -O2 -g -fsanitize=fuzzer,address,undefined \
 *            -I/path/to/quicpro/include fuzz_websocket.c \
 *            /path/to/libquicpro.a -o fuzz_ws
 *
 *      ./fuzz_ws corpus_dir
 *
 *  RFC ANCHORS
 *  -----------
 *  • RFC 6455 §5.5 – Control frames (Ping/Pong/Close)
 *  • RFC 6455 §5.6 – Fragmentation rules
 *  • RFC 9000 §6.9 – Flow control & stream limits
 *  • RFC 9220 §6   – Mapping WebSocket frames to H3 DATAGRAM/STREAM
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#include "quicpro.h"          /* public C API shipped by the engine */

/* ─────────────────────────────  Globals  ───────────────────────────── */

static quicpro_config_t  *g_cfg = NULL;   /* immutable client configuration */
static quicpro_session_t *g_sess = NULL;  /* QUIC session                   */
static quicpro_ws_t      *g_ws = NULL;    /* upgraded WebSocket stream      */
static bool               g_ready = false;

/*
 *  ensure_websocket()
 *  ------------------
 *  Initialise exactly **once**: create a QUIC client, complete the handshake,
 *  and send the HTTP Upgrade to "/chat".  Reusing the *same* g_ws resource
 *  across runs multiplies effective coverage –  masks rotate every frame,
 *  flow-control windows shrink, and server-side echoes interleave with fresh
 *  fuzzer input, all without wasting iterations on boiler-plate handshakes.
 */
static void ensure_websocket(void)
{
    if (g_ready) return;

    g_cfg = quicpro_config_new();
    quicpro_config_set_test_certificate(g_cfg);          /* accept self-signed */
    quicpro_config_set_application_protocol(g_cfg, "h3");
    quicpro_config_set_initial_max_data(g_cfg, 1 << 20); /* 1 MiB connection FC */
    quicpro_config_set_initial_max_stream_data(g_cfg, 1 << 18);

    g_sess = quicpro_client_connect("127.0.0.1", 4433, g_cfg);
    quicpro_session_drive(g_sess, /*timeout_us=*/0);     /* complete handshake */

    g_ws = quicpro_session_ws_upgrade(g_sess, "/chat", /*binary=*/true);
    g_ready = true;
}

/*
 *  drive_event_loop()
 *  ------------------
 *  Helper that pumps the QUIC event-loop until no outgoing datagrams remain.
 *  The loop is intentionally **lossy**: TX packets are dropped on the floor
 *  because we are interested solely in *parsing* code paths, not networking.
 */
static void drive_event_loop(quicpro_session_t *sess)
{
    uint8_t buf[1500];
    size_t  n;

    /* flush outgoing */
    while ((n = quicpro_session_fetch_datagram(sess, buf, sizeof buf)) > 0)
        ;   /* discard */

    /* drain inbound crypto / H3 streams so application callbacks fire */
    quicpro_session_poll(sess, 0);
}

/*
 *  LLVMFuzzerTestOneInput()
 *  ------------------------
 *  1. Parse the prefix (flags, opcode, Δt)
 *  2. If requested, send a Ping frame with a mirror-able payload
 *  3. Deliver the fuzz-payload as **exactly one** data frame (fin may be 0)
 *  4. Optionally send a Close to explore teardown paths
 *  5. Pull echoed frames to tick RX parsing paths
 */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < 4) return 0;           /* must fit header */

    ensure_websocket();

    const uint8_t flags   = data[0];
    const uint8_t opcode  = data[1];
    uint16_t advance_us   = ((uint16_t)data[2]) | ((uint16_t)data[3] << 8);
    const uint8_t *payload = data + 4;
    size_t payload_len    = size - 4;

    /* advance internal timers first (RFC 9000 §6.3 PTO) */
    quicpro_session_tick(g_sess, advance_us);

    /* ① optional Ping – exercise control frame path */
    if (flags & 0x02) {
        quicpro_ws_ping(g_ws, (const char *)payload,
                        payload_len > 125 ? 125 : payload_len);
    }

    /* ② data frame transmit */
    quicpro_ws_send(g_ws,
                    /* fin = */ (flags & 0x01) ? 1 : 0,
                    opcode,
                    payload, payload_len);

    /* ③ teardown? */
    if (flags & 0x04)
        quicpro_ws_close(g_ws, 1000 /* normal close */, NULL, 0);

    /* drive engine so that echo frames reach the parser */
    drive_event_loop(g_sess);

    /* read & discard echoed data to run RFC 6455 parser on RX path */
    uint8_t rxbuf[1024];
    while (quicpro_ws_recv(g_ws, rxbuf, sizeof rxbuf) > 0)
        ;   /* black-hole – crashes show up via ASan */

    return 0;
}

/*  No cleanup – persistent session boosts path exploration rates. */
