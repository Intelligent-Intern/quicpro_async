/*
 * ─────────────────────────────────────────────────────────────────────────────
 *  FILE: fuzz_client_behavior.c
 *  TARGET: libFuzzer / clang-11+        (compile with -fsanitize=fuzzer,address)
 *
 *  PURPOSE
 *  -------
 *  This harness stress-tests the **client-side packet parsing pipeline** of the
 *  quicpro core engine. Fuzzer-supplied bytes are treated as a sequence of QUIC
 *  datagrams that arrive **after** the Initial + Handshake flights have already
 *  completed. All state machines therefore run in their *most permissive* mode,
 *  which maximises exercised code paths while still producing deterministic
 *  progress for coverage analysis.
 *
 *  WHY THESE EXACT HOOKS?
 *  ----------------------
 *  • `quicpro_session_feed_datagram()`  — consumes raw UDP payloads and covers
 *    frame decoding, flow-control accounting, and key phase transitions.
 *  • `quicpro_session_tick()`           — drives loss-recovery & PTO timers so
 *    corner cases such as duplicate ACKs or spurious timeouts manifest early.
 *
 *  The harness asserts that *no* memory corruption, OOB reads, or unbounded
 *  resource growth occur. Secondary invariants like “congestion window not
 *  negative” are left to the QUIC spec’s internal DEBUG asserts when compiled
 *  with `-DQUIC_DEBUG_ASSERTS`.
 *
 *  REFERENCES
 *  ----------
 *  • RFC 9000 §13  — Packet number space & decryption ordering
 *  • RFC 9002 §6   — Loss detection and probe-timeout (PTO) machinery
 *  • libFuzzer design doc — https://llvm.org/docs/LibFuzzer.html
 *
 *  BUILD & RUN
 *  -----------
 *      clang -O2 -g -fsanitize=fuzzer,address,undefined \
 *            -I/path/to/quicpro/include fuzz_client_behavior.c \
 *            /path/to/libquicpro.a -o fuzz_client_behavior
 *
 *      ./fuzz_client_behavior corpus_dir
 *
 *  The corpus directory may start empty; libFuzzer will auto-populate it.
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include "quicpro.h"            /* ← public C API of the engine    */

/*  A single global session is reused across iterations so that coverage
 *  feedback accounts for long-lived state transitions such as key updates.
 *  The handle is lazily initialised inside the fuzzer entry point because
 *  that simplifies integration when the harness is linked into OSS-Fuzz.
 * ---------------------------------------------------------------------- */
static quicpro_session_t *g_sess = NULL;

/*  libFuzzer entry — called once per input.  Must be *total* in the sense
 *  that it never leaks resources or leaves globals in an undefined state. */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /*  Initialise the QUIC client on first invocation.  A short, fixed
     *  handshake transcript is replayed so that subsequent datagrams are
     *  interpreted with full 1-RTT keys — this hits the widest range of
     *  decrypt-and-parse logic.                                        */
    if (g_sess == NULL) {
        quicpro_config_t *cfg = quicpro_config_new();
        /*  Safe defaults: disable verification & logging for speed.   */
        quicpro_config_set_verify_peer(cfg, false);
        quicpro_config_set_qlog_path(cfg, NULL);

        g_sess = quicpro_session_connect(
            /* host     */ "127.0.0.1",
            /* port     */ 4433,
            /* config   */ cfg);

        /*  A synthetic handshake is injected so the session moves to
         *  state “connected”.  Failing early would skew coverage and
         *  waste CPU cycles.                                          */
        if (!quicpro_session_force_handshake_complete(g_sess)) {
            abort();            /* unreachable unless engine broke */
        }

        quicpro_config_free(cfg);
    }

    /*  Defensive barrier: libFuzzer sometimes hands over enormous blobs
     *  that waste time in crypto. Truncate to 4 * 1350 bytes so that the
     *  fuzzer explores *sequences* of packets rather than single MTU-
     *  sized monsters.                                                 */
    if (size > 5400) size = 5400;

    /*  Feed fuzzer data into the engine exactly as if it were received
     *  over the wire. The helper internally loops over datagrams that
     *  are split on 1350-byte boundaries, mirroring a realistic path
     *  through UDP + GSO aggregation.                                  */
    quicpro_session_feed_datagram(g_sess, data, size);

    /*  Advance wall-clock by a random delta from the input stream so
     *  timer-based transitions evolve differently between iterations.
     *  This technique has proven to greatly increase edge coverage on
     *  the PTO and loss-recovery state machines.                       */
    uint64_t advance_us = 0;
    if (size >= 8) { memcpy(&advance_us, data + size - 8, 8); }
    quicpro_session_tick(g_sess, advance_us % 5'000'000); /* ≤ 5 s */

    /*  The harness *must not* close the global session because doing so
     *  would free keys and other long-lived structures that subsequent
     *  fuzz iterations rely on for deeper exploration.                 */
    return 0;
}

/*
 *  A small destructor makes ASan + libFuzzer report memory leaks at the end
 *  of each run instead of suppressing them due to intentionally leaked
 *  globals. This way true leaks in the QUIC implementation still surface.
 */
__attribute__((destructor))
static void fuzz_quic_cleanup(void)
{
    if (g_sess) {
        quicpro_session_close(g_sess);
        quicpro_session_free(g_sess);
        g_sess = NULL;
    }
}
