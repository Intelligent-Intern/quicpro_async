#include "php_quicpro.h"
#include <errno.h>
#ifndef PHP_WIN32
# include <sys/time.h>
# include <unistd.h>
#endif
#include <Zend/zend_fibers.h>

/* ------------------------------------------------------------- */
/* unified helpers for PHP‑8.1‥8.4                               */
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
/* ------------------------------------------------------------- */

static inline void quicpro_yield(void)
{
    if (FIBER_IS_ACTIVE()) FIBER_SUSPEND();
}

PHP_FUNCTION(quicpro_poll)
{
    zval *zsess; zend_long timeout;
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "rl", &zsess, &timeout) == FAILURE)
        RETURN_FALSE;

    quicpro_session_t *s =
        zend_fetch_resource(Z_RES_P(zsess), "quicpro", le_quicpro);

    int64_t q_to = quiche_conn_timeout_as_millis(s->conn);
    if (q_to >= 0 && (timeout < 0 || q_to < timeout)) timeout = q_to;

    fd_set rfds, wfds; FD_ZERO(&rfds); FD_ZERO(&wfds);
    FD_SET(s->sock, &rfds); FD_SET(s->sock, &wfds);

    struct timeval tv, *tvp = NULL;
    if (timeout >= 0) {
        tv.tv_sec  = timeout / 1000;
        tv.tv_usec = (timeout % 1000) * 1000;
        tvp = &tv;
    }

    int sel = select(s->sock + 1, &rfds, &wfds, NULL, tvp);
    if (sel < 0 && errno != EINTR) RETURN_FALSE;
    if (sel == 0) quiche_conn_on_timeout(s->conn);

    /* ------------ read ------------ */
    if (FD_ISSET(s->sock, &rfds)) {
        for (;;) {
            uint8_t buf[65535];
            struct sockaddr_storage from; socklen_t flen = sizeof(from);
            ssize_t n = recvfrom(s->sock, buf, sizeof(buf), MSG_DONTWAIT,
                                 (struct sockaddr *)&from, &flen);
            if (n <= 0) break;

            quiche_recv_info ri = {
                .from     = (struct sockaddr *)&from,
                .from_len = flen,
                .to       = NULL,
                .to_len   = 0
            };
            quiche_conn_recv(s->conn, buf, n, &ri);
        }
    }

    /* ------------ write ----------- */
    for (;;) {
        uint8_t out[1350];
        quiche_send_info si;
        ssize_t n = quiche_conn_send(s->conn, out, sizeof(out), &si);
        if (n <= 0) break;
        if (si.to_len)
            sendto(s->sock, out, n, 0,
                   (const struct sockaddr *)&si.to, si.to_len);
    }

    quicpro_yield();
    RETURN_TRUE;
}