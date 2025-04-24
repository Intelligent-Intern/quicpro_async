/*  Event‑loop helper for php‑quicpro
 *  -------------------------------------------------------------
 *  Implements adaptive busy‑polling, optional AF_XDP receive path,
 *  kernel‑timestamp collection and fiber integration.
 */

#include "php_quicpro.h"

#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <linux/net_tstamp.h>
#include <linux/errqueue.h>

#if defined(__linux__)
# define QUICPRO_HAVE_NAPI_BUSY_POLL 1
#endif

/* -------------------------------------------------------------------------
 *  Fiber helpers – PHP >= 8.1
 * ------------------------------------------------------------------------- */
#if PHP_VERSION_ID >= 80400
# define FIBER_IS_ACTIVE()   (zend_fiber_get_current() != NULL)
# define FIBER_SUSPEND()     zend_fiber_suspend(NULL, NULL, NULL)
#elif PHP_VERSION_ID >= 80100
# define FIBER_IS_ACTIVE()   (EG(current_fiber) != NULL)
# define FIBER_SUSPEND()     zend_fiber_suspend(NULL, NULL)
#else
# define FIBER_IS_ACTIVE()   0
# define FIBER_SUSPEND()
#endif

static inline void quicpro_yield_if_needed(int budget_us)
{
    if (budget_us <= 0 && FIBER_IS_ACTIVE()) {
        FIBER_SUSPEND();
    }
}

/* -------------------------------------------------------------------------
 *  NAPI busy‑poll budget (micro‑seconds)
 * ------------------------------------------------------------------------- */
static int quicpro_busy_budget_us(void)
{
#ifdef QUICPRO_HAVE_NAPI_BUSY_POLL
    static _Atomic int cached = -1;
    int val = atomic_load(&cached);
    if (val >= 0) return val;

    int fd = open("/sys/kernel/net/napi_busy_poll", O_RDONLY);
    if (fd < 0) { atomic_store(&cached, 0); return 0; }
    char buf[32] = {0};
    ssize_t n = read(fd, buf, sizeof(buf)-1);
    close(fd);
    if (n <= 0) { atomic_store(&cached, 0); return 0; }
    val = atoi(buf);
    atomic_store(&cached, val);
    return val;
#else
    return 0;
#endif
}

/* -------------------------------------------------------------------------
 *  Error logging helper – translate select/recv errno to PHP warning.
 * ------------------------------------------------------------------------- */
static inline void quicpro_perror(const char *ctx)
{
    php_error_docref(NULL, E_WARNING, "%s failed: %s", ctx, strerror(errno));
}

/* -------------------------------------------------------------------------
 *  Optional AF_XDP receive helper stubs (compile with -DQUICPRO_XDP)
 * ------------------------------------------------------------------------- */
#ifdef QUICPRO_XDP
#include <bpf/xsk.h>
static ssize_t quicpro_xdp_drain(quicpro_session_t *s)
{
    size_t packets = 0;
    struct xdp_desc *d;
    uint32_t idx;
    while (xsk_ring_cons__peek(&s->rx, 64, &idx) == 64) {
        for (unsigned i = 0; i < 64; i++) {
            d = xsk_ring_cons__comp_addr(&s->rx, idx + i);
            uint8_t *buf = xsk_umem__get_data(s->umem, d->addr);
            quiche_recv_info ri = {
                .from = (struct sockaddr *)&s->peer,
                .from_len = sizeof(s->peer),
                .to = NULL,
                .to_len = 0};
            quiche_conn_recv(s->conn, buf, d->len, &ri);
            packets++;
        }
        xsk_ring_cons__release(&s->rx, 64);
    }
    return packets;
}
#endif /* QUICPRO_XDP */

/* -------------------------------------------------------------------------
 *  quicpro_poll()  –  Userland facing function
 * ------------------------------------------------------------------------- */
PHP_FUNCTION(quicpro_poll)
{
    zval *zsess;
    zend_long timeout_ms;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "rl", &zsess, &timeout_ms) == FAILURE) {
        RETURN_FALSE;
    }

    quicpro_session_t *s = zend_fetch_resource(Z_RES_P(zsess), "quicpro", le_quicpro);
    if (!s) RETURN_FALSE;

    int64_t quic_deadline = quiche_conn_timeout_as_millis(s->conn);
    if (quic_deadline >= 0 && (timeout_ms < 0 || quic_deadline < timeout_ms)) {
        timeout_ms = quic_deadline;
    }

    /* ----------------------------------------------------------------- */
    /* Enable kernel timestamping (once per socket)                      */
    if (!s->ts_enabled) {
        int flags = SOF_TIMESTAMPING_SOFTWARE | SOF_TIMESTAMPING_RX_SOFTWARE | SOF_TIMESTAMPING_TX_SOFTWARE | SOF_TIMESTAMPING_RAW_HARDWARE;
        if (setsockopt(s->sock, SOL_SOCKET, SO_TIMESTAMPING_NEW, &flags, sizeof(flags)) == 0) {
            s->ts_enabled = 1;
        }
    }

    /* ----------------------------------------------------------------- */
    /* Adaptive busy‑poll budget                                          */
    int budget_us = quicpro_busy_budget_us();
    struct timespec start_ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start_ts);

    while (1) {
        /* --- RX PATH -------------------------------------------------- */
#ifdef QUICPRO_XDP
        ssize_t x_packets = quicpro_xdp_drain(s);
        if (x_packets > 0) {
            /* immediate loop – account for time later */
        }
#endif
        /* fallback recvmsg() path */
        uint8_t buf[65535];
        struct sockaddr_storage from;
        struct iovec iov = { .iov_base = buf, .iov_len = sizeof(buf) };
        char cbuf[512];
        struct msghdr msg = {
            .msg_name = &from,
            .msg_namelen = sizeof(from),
            .msg_iov = &iov,
            .msg_iovlen = 1,
            .msg_control = cbuf,
            .msg_controllen = sizeof(cbuf),
        };
        ssize_t n = recvmsg(s->sock, &msg, MSG_DONTWAIT);
        if (n > 0) {
            quiche_recv_info ri = {
                .from = (struct sockaddr *)&from,
                .from_len = msg.msg_namelen,
                .to = NULL,
                .to_len = 0};
            quiche_conn_recv(s->conn, buf, n, &ri);
            /* timestamp extraction */
            for (struct cmsghdr *cm = CMSG_FIRSTHDR(&msg); cm; cm = CMSG_NXTHDR(&msg, cm)) {
                if (cm->cmsg_level == SOL_SOCKET && cm->cmsg_type == SO_TIMESTAMPING_NEW) {
                    struct timespec *ts = (struct timespec *)CMSG_DATA(cm);
                    s->last_rx_ts = *ts; /* stats field in session struct */
                    break;
                }
            }
        } else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            quicpro_perror("recvmsg");
        }

        /* --- TX PATH -------------------------------------------------- */
        while (1) {
            uint8_t out[1350];
            quiche_send_info si;
            ssize_t out_len = quiche_conn_send(s->conn, out, sizeof(out), &si);
            if (out_len <= 0) break;
            ssize_t sent = sendto(s->sock, out, out_len, 0,
                                  (const struct sockaddr *)&si.to, si.to_len);
            if (sent < 0) {
                quicpro_perror("sendto");
                break;
            }
        }

        /* --- QUIC timeout -------------------------------------------- */
        int64_t now = quiche_time_millis();
        if (quiche_conn_is_inactive(s->conn)) break;
        if (quiche_conn_timeout_as_millis(s->conn) == 0) {
            quiche_conn_on_timeout(s->conn);
        }

        /* --- Busy‑poll budget check ----------------------------------- */
        struct timespec now_ts;
        clock_gettime(CLOCK_MONOTONIC_RAW, &now_ts);
        int elapsed_us = (now_ts.tv_sec - start_ts.tv_sec) * 1000000 +
                         (now_ts.tv_nsec - start_ts.tv_nsec) / 1000;
        int remaining = budget_us - elapsed_us;
        if (remaining <= 0) {
            quicpro_yield_if_needed(remaining);
            break; /* return to userland */
        }
    }

    /* refresh ticket cache */
    s->ticket_len = quiche_conn_get_tls_ticket(s->conn, s->ticket, sizeof(s->ticket));

    RETURN_TRUE;
}
