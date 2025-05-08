/*
 * ─────────────────────────────────────────────────────────────────────────────
 *  FILE: fuzz_server_behavior.c
 *  TARGET: libFuzzer / clang-11+   (compile with -fsanitize=fuzzer,address)
 *
 *  PURPOSE
 *  -------
 *  This harness pounds the **server-side** of *quicpro* with arbitrary
 *  UDP datagrams.  Where *fuzz_quic_stream.c* looks at application-layer
 *  flow on an established connection, this file stresses every stage from
 *  a *listener’s* viewpoint:
 *
 *      • QUIC long-header parsing, Version Negotiation and Retry logic
 *      • TLS 1.3 handshake in QUIC transport parameters (RFC 9001 §4)
 *      • HTTP/3 control streams plus SETTINGS synchronisation (RFC 9114 §7)
 *      • Graceful and abrupt CONNECTION_CLOSE behaviour (RFC 9000 §10.3)
 *
 *  INPUT-TO-STATE MAPPING
 *  ----------------------
 *  The fuzzer sees **raw UDP payloads**.  libFuzzer often uncovers edge-cases
 *  only when packet timing changes, therefore the first two bytes encode a
 *  *virtual* “advance-us” timespan that shifts internal PTO & loss timers:
 *
 *      bytes 0-1   →  Δtime in microseconds  (uint16, little-endian)
 *      bytes 2-†   →  untrusted datagram delivered to the server socket
 *
 *  BUILD & RUN
 *  -----------
 *      clang -O2 -g -fsanitize=fuzzer,address,undefined \
 *            -I/path/to/quicpro/include fuzz_server_behavior.c \
 *            /path/to/libquicpro.a -o fuzz_server_behavior
 *
 *      # Provide at least one well-formed seed packet for coverage start-up
 *      ./fuzz_server_behavior corpus_dir
 *
 *  REFERENCES
 *  ----------
 *  • RFC 9000  – QUIC transport
 *  • RFC 9001  – QUIC-TLS handshake mapping
 *  • RFC 9114  – HTTP/3
 *
 *  The constant citations sprinkled through the code point maintainers to
 *  the normative requirement tested by the respective block.
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>

#include "quicpro.h"              /* public C API */

static quicpro_server_t *g_srv      = NULL;
static quicpro_config_t *g_cfg      = NULL;
static bool              g_inited   = false;

/*
 *  INITIALISATION
 *  --------------
 *  We build exactly one in-memory server instance for the whole fuzzing
 *  campaign.  libFuzzer forks worker processes, therefore leaking the objects
 *  here is fine and avoids costly re-handshakes every iteration.
 *
 *  The endpoint binds to an *ephemeral* port on 127.0.0.1 that never receives
 *  traffic from the outside world; all packets originate from the fuzzer.
 *  RFC 9000 requires servers to advertise sensible flow-control credits
 *  immediately, so configuration mirrors a production deployment.
 */
static void ensure_server(void)
{
    if (g_inited) return;

    g_cfg = quicpro_config_new();
    quicpro_config_set_test_certificate(g_cfg);          /* allow self-signed */
    quicpro_config_set_initial_max_data(g_cfg, 4 * 1024 * 1024);
    quicpro_config_set_initial_max_stream_data(g_cfg, 512 * 1024);

    g_srv = quicpro_server_listen("127.0.0.1", 0 /* any port */, g_cfg);

    g_inited = true;
}

/*
 *  HELPERS
 *  -------
 *  Drains every active session so frame parsers execute fully.  No mutation
 *  of application state happens; we only read and discard to maximise code
 *  coverage and unveil use-after-free or overflow bugs in parsing stacks.
 */
static void pump_server(quicpro_server_t *srv)
{
    /* RFC 9000 §6.9 – Accept zero or more connections that completed handshake */
    quicpro_session_t *sess;
    while ((sess = quicpro_server_accept(srv)) != NULL) {
        /* RFC 9114 §6  – Process uni/bi-directional streams until empty      */
        uint64_t sid;
        while (quicpro_session_next_stream(sess, &sid)) {
            uint8_t sink[256];
            ssize_t rd;
            while ((rd = quicpro_stream_recv(sess, sid, sink, sizeof sink)) > 0)
                ;  /* swallow application data */
        }
    }
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < 3) return 0;          /* need at least Δt + 1 payload byte */
    ensure_server();

    /* 1️⃣  extract virtual time-jump Δt (RFC 9000 §6.2.1 – loss timer) */
    uint16_t advance_us = 0;
    memcpy(&advance_us, data, 2);

    /* 2️⃣  feed the remaining buffer as one UDP datagram (RFC 9000 §12) */
    const uint8_t *packet = data + 2;
    size_t         len    = size - 2;

    quicpro_addr_t peer = { .ip = 0x0A000001, .port = 54321 }; /* 10.0.0.1:54321 */
    quicpro_server_ingest_datagram(g_srv, packet, len, &peer);

    /* 3️⃣  advance timers so PTO, ACK-eliciting logic & idle timeouts run */
    quicpro_server_tick(g_srv, (uint64_t)advance_us);

    /* 4️⃣  drain sessions & streams to exercise higher layers             */
    pump_server(g_srv);

    return 0;
}

/*
 *  No explicit destructor is provided.  libFuzzer keeps the server process
 *  alive between iterations and even between crashes when *minimising*
 *  interesting inputs.  Persisting the server therefore increases coverage
 *  and keeps its QPACK dynamic tables in a complex state across runs.
 */
