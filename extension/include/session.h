/*  session.h  ─  in-memory representation of a single QUIC session
 *  =================================================================
 *
 *  Big picture
 *  -----------
 *  A *session* stands for “everything we need to keep a connection to
 *  one HTTP/3 origin alive”.  That includes:
 *
 *     • A UDP socket
 *     • quiche’s TLS + QUIC connection handles
 *     • A little bookkeeping for tickets, timing and statistics
 *
 *  Why put all of this in one struct?
 *  ----------------------------------
 *  PHP calls into C many times per request (connect, send, poll, …).
 *  By tucking every native pointer and counter into *one* object we
 *  hand PHP a single opaque resource (`\Quicpro\Session`) and avoid a
 *  mess of global variables.
 *
 *  Thread-safety note
 *  ------------------
 *  The Zend engine never lets the same resource escape into two
 *  concurrent threads, so we don’t need mutexes *inside* the struct.
 *  When FPM uses multiple workers each one gets its own copy.
 */

#ifndef QUICPRO_SESSION_H
#define QUICPRO_SESSION_H

#include <quiche.h>             /* core QUIC / HTTP-3 implementation */
#include <sys/socket.h>         /* socket(), sockaddr_storage         */
#include <time.h>               /* timespec               (rx stamp) */
#include <stdint.h>
#include <stdatomic.h>          /* ticket ring uses C11 atomics       */

/*--------------------------------------------------------------------*/
/* 1.  Forward declarations                                           */
/*--------------------------------------------------------------------*/

/*  Borrowed from php_quicpro.h so callers need only one #include.  */
typedef struct quicpro_ticket_ring_s quicpro_ticket_ring_t;

/*--------------------------------------------------------------------*/
/* 2.  Session object                                                 */
/*--------------------------------------------------------------------*/

typedef struct quicpro_session_s {
    /*----------------------------------------------------------------
     *  2.1  Transport handles
     *--------------------------------------------------------------*/
    int              sock;              /* UDP socket FD (-1 = none)     */
    quiche_conn     *conn;              /* live QUIC connection          */
    quiche_h3_conn  *h3;                /* HTTP/3 stream multiplexor     */

    /*----------------------------------------------------------------
     *  2.2  Configuration (shared, read-only)
     *--------------------------------------------------------------*/
    const quiche_config    *cfg;        /* points to global cfg resource */
    quiche_h3_config       *h3_cfg;     /* per-session copy (quiche API) */

    /*----------------------------------------------------------------
     *  2.3  Identity
     *--------------------------------------------------------------*/
    char             host[256];         /* origin host (ASCII, NUL-term) */
    char             sni_host[256];     /* Server Name Indication        */
    uint8_t          scid[16];          /* source connection ID          */

    /*----------------------------------------------------------------
     *  2.4  Session resumption (0-RTT)
     *--------------------------------------------------------------*/
    uint8_t          ticket[512];       /* latest received ticket        */
    uint32_t         ticket_len;        /* 0 ⇒ no ticket yet             */

    /*----------------------------------------------------------------
     *  2.5  Timing / diagnostics
     *--------------------------------------------------------------*/
    struct timespec  last_rx_ts;        /* kernel timestamp of last pkt  */
    uint8_t          ts_enabled;        /* did we setsockopt SO_TS?      */
    int              numa_node;         /* socket-CPU affinity hint      */

    /*----------------------------------------------------------------
     *  2.6  Linked list pointer (optional pooling)
     *--------------------------------------------------------------*/
    struct quicpro_session_s *next;
} quicpro_session_t;

/*--------------------------------------------------------------------*/
/* 3.  Helper API (implemented in session.c)                           */
/*--------------------------------------------------------------------*/

/*
 *  quicpro_session_open()
 *  ----------------------
 *  • Allocates & initialises a session object
 *  • Performs socket() + connect() + quiche_connect()
 *  • Returns NULL and sets quicpro_last_error on failure
 */
quicpro_session_t *
quicpro_session_open(const char        *host,
                     uint16_t           port,
                     const quiche_config *cfg,
                     int                numa_node);

/*  Gracefully close the connection and free all native handles.     */
void quicpro_session_close(quicpro_session_t *s);

/*
 *  quicpro_session_export_ticket()
 *  -------------------------------
 *  Called after a successful handshake to share the 0-RTT ticket
 *  with sibling PHP-FPM workers.  Copies the bytes into the global
 *  ring buffer declared in tls.c.
 */
void quicpro_session_export_ticket(quicpro_session_t *s);

/*--------------------------------------------------------------------*/
/* 4.  perf_event integration helper (Linux only)                     */
/*--------------------------------------------------------------------*/

#ifdef __linux__
/*
 *  quicpro_enable_kernel_timestamps()
 *  ----------------------------------
 *  Requests hardware-level send/recv timestamps via SO_TIMESTAMPING.
 *  The first call pays the setsockopt() cost; subsequent calls are
 *  no-ops.  Result is cached in s->ts_enabled.
 */
int  quicpro_enable_kernel_timestamps(quicpro_session_t *s);
#endif

#endif /* QUICPRO_SESSION_H */
