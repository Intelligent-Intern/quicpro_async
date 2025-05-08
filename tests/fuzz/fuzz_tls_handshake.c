/*
 * ─────────────────────────────────────────────────────────────────────────────
 *  FILE: fuzz_tls_handshake.c
 *  TARGET: libFuzzer / clang-11+   (compile with -fsanitize=fuzzer,address)
 *
 *  PURPOSE
 *  -------
 *  This harness bombards the **TLS 1.3 handshake inside QUIC** from a
 *  *client-side* perspective.  The goal is to exhaustively exercise the state
 *  machine that maps encrypted QUIC packets to TLS records as defined in
 *  RFC 9001 §4, while simultaneously stressing the transport’s loss-recovery
 *  and timer logic from RFC 9000 §6.2-6.3.  A single persistent connection
 *  amplifies coverage because the cryptographic context (keys, secrets,
 *  traffic limits) evolves over time and survives across fuzz iterations.
 *
 *  INPUT MODEL
 *  -----------
 *  The fuzzer supplies **one UDP datagram** per call.  To reach code paths
 *  gated by timers (e.g. Probe Timeout), the first two bytes encode a virtual
 *  time shift that moves internal clocks forward before the packet lands:
 *
 *        bytes 0-1   →  advance_us  (uint16, little-endian micro-seconds)
 *        bytes 2-†   →  arbitrary datagram delivered **from the server**
 *
 *  BUILD & RUN
 *  -----------
 *      clang -O2 -g -fsanitize=fuzzer,address,undefined \
 *            -I/path/to/quicpro/include fuzz_tls_handshake.c \
 *            /path/to/libquicpro.a -o fuzz_tls_handshake
 *
 *      # Seed corpus with at least one valid Initial packet for faster start
 *      ./fuzz_tls_handshake corpus_dir
 *
 *  RFC ANCHORS
 *  -----------
 *  • RFC 9001 §4.1-4.10 – QUIC ↔ TLS handshake mapping rules
 *  • RFC 9000 §7.4       – Retry & Version Negotiation defenses
 *  • RFC 8446 §4         – TLS 1.3 handshake transcript validation
 *
 *  Each block below cites the relevant normative text so maintainers know
 *  *why* a step exists when coverage reports change after edits upstream.
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#include "quicpro.h"                /* public C API – shipped with the engine */

/* ───────────────────────────── Globals ───────────────────────────── */

static quicpro_config_t  *g_cfg   = NULL;   /* immutable configuration       */
static quicpro_session_t *g_cli   = NULL;   /* single QUIC client instance   */
static bool               g_init  = false;  /* one-time initialisation guard */

/*
 *  ensure_client()
 *  ---------------
 *  Spin up a *single* QUIC client equipped with a self-signed certificate so
 *  that the handshake can complete without external PKI infrastructure.  The
 *  connection targets 127.0.0.1:4433, yet never transmits packets over the
 *  real network – every datagram originates from the fuzzer process.
 *
 *  Persisting the client across iterations is intentional: RFC 9001 demands
 *  that key update limits (≤ 2^23 packets, §5.4) and anti-amplification rules
 *  accumulate state over the lifetime of a connection.  Reusing the session
 *  therefore explores vastly more code with fewer iterations.
 */
static void ensure_client(void)
{
    if (g_init) return;

    g_cfg = quicpro_config_new();
    quicpro_config_set_test_certificate(g_cfg);          /* self-signed OK   */
    quicpro_config_set_application_protocol(g_cfg, "h3");/* RFC 9114 §3      */
    quicpro_config_set_initial_max_data(g_cfg, 1024 * 1024);
    quicpro_config_set_initial_max_stream_data(g_cfg, 256 * 1024);

    g_cli = quicpro_client_connect("127.0.0.1", 4433, g_cfg);
    g_init = true;
}

/*
 *  drain_crypto_streams()
 *  ---------------------
 *  Pull crypto frames until none are left so that the TLS stack fully parses
 *  ClientHello/ServerHello, EncryptedExtensions, and finished messages.  This
 *  reaches code that verifies transcript hashes (RFC 8446 §4.4.1) and applies
 *  QUIC-specific key updates (RFC 9001 §5.2).
 */
static void drain_crypto_streams(quicpro_session_t *cli)
{
    uint64_t sid = 0;
    while (quicpro_session_next_crypto_stream(cli, &sid)) {
        uint8_t buf[512];
        while (quicpro_stream_recv(cli, sid, buf, sizeof buf) > 0)
            ; /* consume & discard; goal is coverage, not correctness */
    }
}

/*
 *  LLVMFuzzerTestOneInput()
 *  ------------------------
 *  libFuzzer’s entry point.  Each call represents one *logical* point in the
 *  timeline of the virtual connection: advance timers, deliver a datagram,
 *  run handshake/crypto processing, repeat.
 *
 *  The harness is deliberately **silent** on errors – any misbehaviour shows
 *  up as ASan/UBSan findings or segmentation faults, which libFuzzer treats
 *  as crashes to minimise and report.
 */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < 3) return 0;         /* need Δt + ≥1 payload byte */

    ensure_client();

    /* 1️⃣  extract virtual time delta (RFC 9000 §6.3 – PTO timer)          */
    uint16_t advance_us;
    memcpy(&advance_us, data, 2);
    quicpro_session_tick(g_cli, (uint64_t)advance_us);

    /* 2️⃣  ingest the forged datagram as if it came from the server        */
    const uint8_t *packet = data + 2;
    size_t         len    = size - 2;

    quicpro_addr_t peer = { .ip = 0x7F000001 /*127.0.0.1*/, .port = 4433 };
    quicpro_session_ingest_datagram(g_cli, packet, len, &peer);

    /* 3️⃣  drive the TLS record layer & transport loss recovery           */
    drain_crypto_streams(g_cli);

    /* 4️⃣  Optionally emit client-generated datagrams to hit TX paths.    */
    uint8_t out[1350];
    size_t  out_len;
    while ((out_len = quicpro_session_fetch_datagram(g_cli, out, sizeof out))
           > 0) {
        /* We discard client packets – feedback loop isn’t required for
         * uncovering parsing bugs. */
    }

    return 0;
}

/*
 *  Cleanup is intentionally omitted.  Keeping the connection alive across
 *  fuzz iterations raises the internal epoch counters so cipher re-keys
 *  (RFC 9001 §5.2) and packet number encoding (RFC 9000 §12.3) happen under
 *  wildly varying conditions, which in turn multiplies effective coverage.
 */
