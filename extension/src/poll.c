/*
 * poll.c  –  Adaptive event-loop helper for php-quicpro
 *
 * Responsibilities:
 *   • Drain incoming QUIC packets (recvmsg() or optional AF_XDP fast-path).
 *   • Push outgoing packets produced by quiche.
 *   • Honor quiche’s connection/idle time-outs.
 *   • Expose kernel RX/TX timestamps via SO_TIMESTAMPING_NEW.
 *   • Respect the system-wide NAPI busy-poll budget (net.core.busy_poll).
 *   • Cooperate with PHP Fibers: if busy-poll budget is exceeded inside a fiber,
 *     yield to the scheduler.
 *
 * Build-time dependencies:
 *   • <quiche.h>             – transitively pulled in via php_quicpro.h
 *   • "session.h"            – defines quicpro_session_t and extern int le_quicpro_session
 *   • "php_quicpro_arginfo.h"– arginfo_quicpro_poll
 *   • <Zend/zend_fibers.h>   – declares EG(current_fiber), zend_fiber_suspend() (PHP ≥ 8.1)
 */

#include "session.h"             /* quicpro_session_t, extern int le_quicpro_session */
#include "php_quicpro.h"
#include <quiche.h>

/* ───────────────────────────────── Fiber support ────────────────────────── */
/* PHP ≥ 8.4: zend_fiber_get_current() is public but not always in installed headers,
   so we declare it manually. PHP 8.1–8.3: use EG(current_fiber). PHP < 8.1: no fibers. */
#if PHP_VERSION_ID >= 80400
typedef struct _zend_fiber zend_fiber;
extern zend_fiber *zend_fiber_get_current(void);
extern void zend_fiber_suspend(zend_fiber *fiber, zval *value, zval *return_value);
# define FIBER_GET_CURRENT()   zend_fiber_get_current()
#elif PHP_VERSION_ID >= 80100
# include <Zend/zend_fibers.h>
# define FIBER_GET_CURRENT()   (EG(current_fiber))
#else
# define FIBER_GET_CURRENT()   NULL
#endif

#define FIBER_IS_ACTIVE()   (FIBER_GET_CURRENT() != NULL)
#define FIBER_SUSPEND()     zend_fiber_suspend(FIBER_GET_CURRENT(), NULL, NULL)

/* ─────────────────────── quiche compatibility shims ────────────────────── */
#ifndef quiche_conn_get_tls_ticket
# define quiche_conn_get_tls_ticket(_c,_b,_l) 0
#endif
#ifndef quiche_conn_is_inactive
# define quiche_conn_is_inactive quiche_conn_is_draining
#endif

#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>
#include <linux/net_tstamp.h>
#include <linux/errqueue.h>

#ifdef QUICPRO_HAVE_NAPI_BUSY_POLL
# include <stdatomic.h>
#endif

#ifdef QUICPRO_XDP
# include <bpf/xsk.h>
/*
 * Fast-path: drain packets from AF_XDP and feed them to quiche.
 */
static ssize_t quicpro_xdp_drain(quicpro_session_t *s)
{
    size_t packets = 0;
    uint32_t idx;

    while (xsk_ring_cons__peek(&s->rx, 64, &idx) == 64) {
        for (unsigned i = 0; i < 64; i++) {
            struct xdp_desc *d = xsk_ring_cons__comp_addr(&s->rx, idx + i);
            uint8_t *buf = xsk_umem__get_data(s->umem, d->addr);

            quiche_recv_info ri = {
                .from     = (struct sockaddr *)&s->peer,
                .from_len = sizeof(s->peer),
                .to       = NULL,
                .to_len   = 0
            };
            quiche_conn_recv(s->conn, buf, d->len, &ri);
            packets++;
        }
        xsk_ring_cons__release(&s->rx, 64);
    }
    return packets;
}
#endif /* QUICPRO_XDP */

/*
 * Suspend execution if we've exceeded our busy-poll budget and are inside a Fiber.
 */
static inline void quicpro_yield_if_needed(int budget_us)
{
    if (budget_us <= 0 && FIBER_IS_ACTIVE()) {
        FIBER_SUSPEND();
    }
}

/*
 * Returns the NAPI busy-poll budget (µs) or 0 if disabled. Caches the value.
 */
static int quicpro_busy_budget_us(void)
{
#ifdef QUICPRO_HAVE_NAPI_BUSY_POLL
    static _Atomic int cached = -1;
    int val = atomic_load(&cached);
    if (val >= 0) {
        return val;
    }

    int fd = open("/sys/kernel/net/napi_busy_poll", O_RDONLY);
    if (fd < 0) {
        atomic_store(&cached, 0);
        return 0;
    }

    char buf[32] = {0};
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n <= 0) {
        atomic_store(&cached, 0);
        return 0;
    }

    val = atoi(buf);
    atomic_store(&cached, val);
    return val;
#else
    return 0;
#endif
}

/*
 * Emit a PHP warning with strerror(errno).
 */
static inline void quicpro_perror(const char *ctx)
{
    php_error_docref(NULL, E_WARNING, "%s failed: %s", ctx, strerror(errno));
}

/*
 * quicpro_poll(resource $session, int $timeout_ms): bool
 *
 * One iteration of the QUIC event loop:
 *   1) Drain RX (XDP or recvmsg)
 *   2) Send pending packets
 *   3) Handle timeouts
 *   4) Yield to Fiber if budget expired
 *   5) Refresh session ticket cache
 */
PHP_FUNCTION(quicpro_poll)
{
    zval      *zsess;
    zend_long  timeout_ms;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "rl", &zsess, &timeout_ms) == FAILURE) {
        RETURN_FALSE;
    }

    /* Fetch our C-level session object from the PHP resource */
    quicpro_session_t *s =
        zend_fetch_resource(Z_RES_P(zsess), "quicpro", le_quicpro_session);
    if (!s) {
        RETURN_FALSE;
    }

    /* Honor quiche’s suggested next timeout */
    int64_t quic_deadline = quiche_conn_timeout_as_millis(s->conn);
    if (quic_deadline >= 0 &&
       (timeout_ms < 0 || quic_deadline < timeout_ms)) {
        timeout_ms = quic_deadline;
    }

    /* Enable kernel RX/TX timestamping once */
    if (!s->ts_enabled) {
        int flags = SOF_TIMESTAMPING_SOFTWARE
                  | SOF_TIMESTAMPING_RX_SOFTWARE
                  | SOF_TIMESTAMPING_TX_SOFTWARE
                  | SOF_TIMESTAMPING_RAW_HARDWARE;
        if (setsockopt(s->sock, SOL_SOCKET, SO_TIMESTAMPING_NEW,
                       &flags, sizeof(flags)) == 0) {
            s->ts_enabled = 1;
        }
    }

    /* Busy-poll loop bounded by NAPI budget or quiche timeout */
    int            budget_us = quicpro_busy_budget_us();
    struct timespec start_ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start_ts);

    while (1) {
        /* RX path */
    #ifdef QUICPRO_XDP
        quicpro_xdp_drain(s);
    #endif
        {
            uint8_t buf[65535];
            struct sockaddr_storage from;
            struct iovec iov = { .iov_base = buf, .iov_len = sizeof(buf) };
            char cbuf[512];

            struct msghdr msg = {
                .msg_name       = &from,
                .msg_namelen    = sizeof(from),
                .msg_iov        = &iov,
                .msg_iovlen     = 1,
                .msg_control    = cbuf,
                .msg_controllen = sizeof(cbuf),
            };

            ssize_t n = recvmsg(s->sock, &msg, MSG_DONTWAIT);
            if (n > 0) {
                quiche_recv_info ri = {
                    .from     = (struct sockaddr *)&from,
                    .from_len = msg.msg_namelen,
                    .to       = NULL,
                    .to_len   = 0
                };
                quiche_conn_recv(s->conn, buf, n, &ri);

                /* extract kernel RX timestamp, if any */
                for (struct cmsghdr *cm = CMSG_FIRSTHDR(&msg); cm;
                     cm = CMSG_NXTHDR(&msg, cm)) {
                    if (cm->cmsg_level == SOL_SOCKET &&
                        cm->cmsg_type  == SO_TIMESTAMPING_NEW) {
                        s->last_rx_ts = *(struct timespec *)CMSG_DATA(cm);
                        break;
                    }
                }
            } else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                quicpro_perror("recvmsg");
            }
        }

        /* TX path */
        while (1) {
            uint8_t out[1350];
            quiche_send_info si;
            ssize_t out_len =
                quiche_conn_send(s->conn, out, sizeof(out), &si);
            if (out_len <= 0) {
                break;
            }
            ssize_t sent = sendto(s->sock, out, out_len, 0,
                                  (const struct sockaddr *)&si.to,
                                  si.to_len);
            if (sent < 0) {
                quicpro_perror("sendto");
                break;
            }
        }

        /* QUIC timeout handling */
        if (quiche_conn_is_inactive(s->conn)) {
            break;
        }
        if (quiche_conn_timeout_as_millis(s->conn) == 0) {
            quiche_conn_on_timeout(s->conn);
        }

        /* Budget check and possible yield */
        struct timespec now_ts;
        clock_gettime(CLOCK_MONOTONIC_RAW, &now_ts);
        int elapsed_us = (now_ts.tv_sec  - start_ts.tv_sec)  * 1000000
                       + (now_ts.tv_nsec - start_ts.tv_nsec) / 1000;
        if (elapsed_us >= budget_us) {
            quicpro_yield_if_needed(elapsed_us - budget_us);
            break;
        }
    }

    /* Refresh session ticket cache for export_session_ticket() */
    s->ticket_len = quiche_conn_get_tls_ticket(s->conn,
                                              s->ticket,
                                              sizeof(s->ticket));

    RETURN_TRUE;
}
