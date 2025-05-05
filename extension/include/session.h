/*  session.h  –  In‑Memory Representation of a Single QUIC Session
 *  =================================================================
 *
 *  This header defines the core data structure and helper functions for
 *  managing a live QUIC connection in the php‑quicpro_async extension.
 *  A “session” bundles everything needed to open, maintain, and tear down
 *  a connection to one HTTP/3 origin.  By keeping all related state in
 *  a single struct, we provide PHP with an opaque resource handle that
 *  encapsulates sockets, protocol engines, and bookkeeping data.
 *
 *  Conceptual Overview
 *  -------------------
 *  • A session owns a non‑blocking UDP socket bound to the remote server.
 *  • It holds quiche’s TLS+QUIC connection and HTTP/3 stream contexts.
 *  • It tracks session tickets for 0‑RTT resumption and timing metadata.
 *  • All of this is wrapped in a Zend resource so PHP’s GC can close
 *    connections automatically when no longer in use.
 *
 *  Thread Safety
 *  -------------
 *  In typical PHP‑FPM or Apache prefork deployments, each process is
 *  single‑threaded.  This means we can store session state without
 *  internal mutexes.  If the extension is embedded in a multithreaded
 *  SAPI, the engine will not hand the same resource to two threads
 *  concurrently, so our design remains safe.
 */

#ifndef QUICPRO_SESSION_H
#define QUICPRO_SESSION_H

#include <quiche.h>             /* Core QUIC / HTTP/3 implementation */
#include <sys/socket.h>         /* socket(), sockaddr_storage         */
#include <time.h>               /* timespec for kernel timestamps     */
#include <stdint.h>             /* Fixed‑width integer types          */
#include <stdatomic.h>          /* Atomics for ticket ring operations */

/*------------------------------------------------------------------------*/
/* 1. Forward Declarations                                                */
/*------------------------------------------------------------------------*/

/*
 *  quicpro_ticket_ring_t
 *  ----------------------
 *  An opaque ring buffer of session tickets used for sharing TLS tickets
 *  across reconnect attempts or worker processes.  We forward‑declare its
 *  type here so that callers of the API need only include session.h,
 *  without needing to know the ring’s internal layout.
 */
typedef struct quicpro_ticket_ring_s quicpro_ticket_ring_t;

/*------------------------------------------------------------------------*/
/* 2. quicpro_session_t: Core Session Object                              */
/*------------------------------------------------------------------------*/

/**
 * quicpro_session_t
 * -----------------
 * This struct holds all state associated with a single QUIC connection
 * to one HTTP/3 origin.  It is allocated when the session is opened
 * and freed when the PHP resource is garbage‑collected.
 *
 * Grouped Sections:
 *   2.1 Transport Handles
 *   2.2 Configuration References
 *   2.3 Identity & Connection IDs
 *   2.4 Session Resumption Data
 *   2.5 Timing & Diagnostics
 *   2.6 Optional Pooling Link
 */
typedef struct quicpro_session_s {
    /*--------------------------------------------------------------------
     * 2.1 Transport Handles
     *------------------------------------------------------------------*/
    int              sock;    /* POSIX UDP socket FD used for sending/receiving.
                                 * Initialized to -1 until connect succeeds.
                                 * Closed when the session is freed. */
    quiche_conn     *conn;    /* Opaque pointer to quiche’s QUIC connection.
                                 * Manages packet encryption, retransmits,
                                 * and state machines. */
    quiche_h3_conn  *h3;      /* HTTP/3 connection context layered on top of QUIC.
                                 * Multiplexes streams, encodes/decodes headers. */

    /*--------------------------------------------------------------------
     * 2.2 Configuration (shared, read‑only)
     *------------------------------------------------------------------*/
    const quiche_config    *cfg;    /* Pointer to the immutable quiche_config
                                       * object created by quicpro_new_config().
                                       * Shared across multiple sessions. */
    quiche_h3_config       *h3_cfg; /* Per‑session HTTP/3 configuration.
                                       * Created by quiche_h3_config_new(). */

    /*--------------------------------------------------------------------
     * 2.3 Identity
     *------------------------------------------------------------------*/
    char             host[256];     /* Null‑terminated ASCII host name used
                                       * for SNI and :authority header.
                                       * Copied from PHP parameter. */
    char             sni_host[256]; /* Explicit SNI string, if different from host.
                                       * Useful when virtual‑hosting with custom names. */
    uint8_t          scid[16];      /* Source Connection ID chosen by client.
                                       * Random bytes used to identify this session
                                       * to the server. */

    /*--------------------------------------------------------------------
     * 2.4 Session Resumption (0‑RTT)
     *------------------------------------------------------------------*/
    uint8_t          ticket[512];   /* Buffer holding the latest TLS session
                                       * ticket received from the server.
                                       * Used for 0‑RTT handshakes. */
    uint32_t         ticket_len;    /* Length of valid data in ticket[].
                                       * Zero indicates “no ticket available.” */

    /*--------------------------------------------------------------------
     * 2.5 Timing & Diagnostics
     *------------------------------------------------------------------*/
    struct timespec  last_rx_ts;    /* Kernel timestamp of the most recent
                                       * received QUIC packet (if SO_TIMESTAMPING
                                       * is enabled).  Useful for RTT estimation. */
    uint8_t          ts_enabled;    /* Flag indicating whether we have called
                                       * setsockopt to enable timestamping. */
    int              numa_node;     /* Optional CPU affinity hint when creating
                                       * the socket.  -1 if no affinity requested. */

    /*--------------------------------------------------------------------
     * 2.6 Linked List Pointer (Optional Pooling)
     *------------------------------------------------------------------*/
    struct quicpro_session_s *next; /* Pointer to next session in a free pool.
                                      * Null if pooling is not used or at tail. */
} quicpro_session_t;

/*------------------------------------------------------------------------*/
/* 3. Helper API (implemented in session.c)                               */
/*------------------------------------------------------------------------*/

/**
 * quicpro_session_open()
 * ----------------------
 * Allocate and initialize a new quicpro_session_t.  This function:
 *   1. Performs DNS resolution of the host.
 *   2. Opens a non‑blocking UDP socket and connects it to the remote address.
 *   3. Allocates and configures quiche_conn for a new QUIC session.
 *   4. Allocates and configures quiche_h3_conn for HTTP/3 usage.
 *   5. Returns NULL on any failure, setting quicpro_last_error[] for diagnostics.
 *
 * @param host       DNS name or IP string of the server to connect.
 * @param port       Remote UDP port number in host byte order.
 * @param cfg        Pointer to a valid quiche_config instance.
 * @param numa_node  CPU node hint for socket affinity, or -1 to ignore.
 * @return           A pointer to a fully initialized session object, or NULL.
 */
quicpro_session_t *
quicpro_session_open(const char          *host,
                     uint16_t             port,
                     const quiche_config *cfg,
                     int                  numa_node);

/**
 * quicpro_session_close()
 * -----------------------
 * Gracefully close the QUIC connection and free all native resources
 * held by the session struct.  This includes:
 *   • Calling quiche_conn_close() if still active.
 *   • Freeing quiche_h3_conn, quiche_conn, quiche_h3_config.
 *   • Closing the UDP socket.
 *   • Freeing the session struct itself.
 *
 * @param s  The session pointer returned by quicpro_session_open().
 */
void quicpro_session_close(quicpro_session_t *s);

/**
 * quicpro_session_export_ticket()
 * -------------------------------
 * After the TLS handshake completes, extract the session ticket from
 * quiche_conn and enqueue it into the process‑wide ticket ring buffer.
 * This enables subsequent sessions or worker processes to import the
 * ticket for 0‑RTT resumptions.
 *
 * @param s  The session whose ticket should be exported.
 */
void quicpro_session_export_ticket(quicpro_session_t *s);

/*------------------------------------------------------------------------*/
/* 4. Linux‑only perf_event Integration Helpers                            */
/*------------------------------------------------------------------------*/

#ifdef __linux__
/**
 * quicpro_enable_kernel_timestamps()
 * ----------------------------------
 * Request kernel‑level packet timestamping via SO_TIMESTAMPING_NEW.
 * The first call issues a setsockopt on the UDP socket; subsequent
 * calls are no‑ops.  When enabled, incoming packets will carry
 * hardware or software timestamps accessible via recvmsg().
 *
 * @param s  The session whose socket should be configured.
 * @return   Zero on success, or -1 on error (with errno set).
 */
int quicpro_enable_kernel_timestamps(quicpro_session_t *s);
#endif

#endif /* QUICPRO_SESSION_H */
