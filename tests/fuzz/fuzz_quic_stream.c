/*
 * ─────────────────────────────────────────────────────────────────────────────
 *  FILE: fuzz_quic_stream.c
 *  TARGET: libFuzzer / clang-11+      (compile with -fsanitize=fuzzer,address)
 *
 *  PURPOSE
 *  -------
 *  Exercise the **HTTP/3 STREAM layer** of *quicpro* by feeding untrusted
 *  payloads straight into the frame parser and flow-control engine. While
 *  *fuzz_quic_connection.c* proves that the handshake & packet protection are
 *  safe, this harness digs into:
 *
 *      • HTTP/3 frame decoding (HEADERS, DATA, CANCEL_PUSH, …)
 *      • QPACK state-synchronisation between encoder and decoder
 *      • Application-level flow-control & stream-reset handling
 *
 *  INPUT-TO-STATE MAPPING
 *  ----------------------
 *  The first  4 bytes  → stream ID (low-ordered, unidirectional flag kept)
 *  The next   2 bytes  → “advance-us” : virtual time-skip for PTO + deadlines
 *  Remainder bytes     → raw frame payload delivered to the chosen stream
 *
 *  REFERENCE MATERIAL
 *  ------------------
 *  • RFC 9114 §4 – Generic HTTP/3 frame layout & stream states
 *  • RFC 9204 – QPACK dynamic table safety
 *  • RFC 9000 §4.6 – Flow-control & MAX_STREAM_DATA
 *
 *  BUILD & RUN
 *  -----------
 *      clang -O2 -g -fsanitize=fuzzer,address,undefined \
 *            -I/path/to/quicpro/include fuzz_quic_stream.c \
 *            /path/to/libquicpro.a -o fuzz_quic_stream
 *
 *      ./fuzz_quic_stream corpus_dir
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>

#include "quicpro.h"          /* public C API */

static quicpro_session_t *g_sess = NULL;
static bool               g_qpack_inited = false;

/* One-time initialisation: perform a full handshake against the built-in
 * demo server shipped with the engine build.  We reuse the session across
 * iterations so that the dynamic QPACK table persists and odd Huffman states
 * become fuzzable.                                                            */
static void ensure_session(void)
{
    if (g_sess) return;

    quicpro_config_t *cfg = quicpro_config_new();
    quicpro_config_set_test_certificate(cfg);     /* allow self-signed */

    g_sess = quicpro_client_connect("127.0.0.1", 4433, cfg);
    quicpro_config_free(cfg);

    /* Complete handshake once so STREAM frames are accepted. */
    while (!quicpro_session_is_established(g_sess))
        quicpro_session_tick(g_sess, /* advance */ 10'000);

    /* Initialise QPACK encoder/decoder pair exactly once; the table survives
     * between fuzz iterations and therefore widens code coverage.            */
    if (!g_qpack_inited) {
        quicpro_qpack_init(g_sess);
        g_qpack_inited = true;
    }
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < 6) return 0;                 /* need minimum header fields */
    ensure_session();

    /* 1️⃣  derive stream id & virtual time */
    uint32_t stream_id   = 0;
    uint16_t advance_us  = 0;
    memcpy(&stream_id,  data,     4);
    memcpy(&advance_us, data + 4, 2);

    if ((stream_id & 0x3) == 0x2)           /* forbid server-initiated IDs */
        stream_id |= 0x1;                   /* flip to client-initiated */

    /* 2️⃣  feed payload into chosen stream */
    const uint8_t *payload = data + 6;
    size_t         len     = size - 6;

    /* Open the stream lazily if fuzz input addresses one not yet in use.     */
    if (!quicpro_stream_exists(g_sess, stream_id))
        quicpro_stream_open(g_sess, stream_id);

    quicpro_stream_feed(g_sess, stream_id, payload, len);

    /* 3️⃣  drive timers so PTO / flow-control logic executes. */
    quicpro_session_tick(g_sess, (uint64_t)advance_us);

    /* 4️⃣  Read & discard any application data produced (HEADERS decode,
     *     QPACK processing, etc.)—we only care about engine robustness.      */
    uint8_t  sink[256];
    ssize_t  rd;
    while ((rd = quicpro_stream_recv(g_sess, stream_id, sink, sizeof sink)) > 0)
        ; /* swallow */

    return 0;
}

/* No destructor is registered – intentionally leak the live session so that
 * libFuzzer keeps the dynamic QPACK table warm across multiple processes.     */
