/*  HTTP/3 helpers for php‑quicpro
 *  --------------------------------------------------------------
 *  Implements request/response helpers with:
 *   • header‑array bounds check (≤ 64)
 *   • dynamic :authority based on SNI / connect‑host
 *   • data‑frame read‑loop until QUICHE_H3_ERR_DONE
 *   • optional streaming body via user‑supplied callback
 *   • perf‑trace emit on send/recv
 */

#include "php_quicpro.h"
#include <string.h>

#define HDR(n,v) (quiche_h3_header){(uint8_t *)(n), sizeof(n)-1, (uint8_t *)(v), sizeof(v)-1}
#define MAX_HDR 64

/* ---------------------------------------------------------------
 * Helper: push perf‑trace marker (stub in ring.c)
 * ---------------------------------------------------------------*/
static inline void trace_emit(const char *tag, uint64_t sid)
{
    quicpro_trace_emit(tag, sid);
}

/* ---------------------------------------------------------------
 * quicpro_send_request
 * ---------------------------------------------------------------*/
PHP_FUNCTION(quicpro_send_request)
{
    zval *zsess, *harr = NULL, *cb = NULL;
    char *path, *method = "GET", *body = NULL;
    size_t path_len, method_len = 3, body_len = 0;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "rs|assz", &zsess,
                              &path, &path_len,
                              &harr,  /* optional headers array */
                              &method, &method_len, /* optional method */
                              &body, &body_len,     /* optional body */
                              &cb   /* optional streaming callback */) == FAILURE)
        RETURN_FALSE;

    quicpro_session_t *s = zend_fetch_resource(Z_RES_P(zsess), "quicpro", le_quicpro);
    if (!s) RETURN_FALSE;

    quiche_h3_header hdrs[MAX_HDR];
    size_t idx = 0;
#define APPEND(h) do { if (idx >= MAX_HDR) { php_error_docref(NULL,E_WARNING,"too many headers (>%d)",MAX_HDR); RETURN_FALSE; } hdrs[idx++] = (h); } while(0)

    APPEND((quiche_h3_header){(uint8_t *)":method", 7, (uint8_t *)method, method_len});
    APPEND((quiche_h3_header){(uint8_t *)":scheme", 7, (uint8_t *)"https", 5});
    APPEND((quiche_h3_header){(uint8_t *)":path",   5, (uint8_t *)path,  path_len});
    /* dynamic authority from connect‑host */
    APPEND((quiche_h3_header){(uint8_t *)":authority", 10, (uint8_t *)s->sni_host, strlen(s->sni_host)});

    /* user‑supplied extra headers */
    if (harr && Z_TYPE_P(harr) == IS_ARRAY) {
        zend_string *k;
        zval *v;
        ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(harr), k, v) {
            if (!k || Z_TYPE_P(v) != IS_STRING) continue;
            APPEND((quiche_h3_header){(uint8_t *)ZSTR_VAL(k), ZSTR_LEN(k),
                                     (uint8_t *)Z_STRVAL_P(v), Z_STRLEN_P(v)});
        } ZEND_HASH_FOREACH_END();
    }
#undef APPEND

    int64_t sid = quiche_h3_send_request(s->h3, s->conn, hdrs, idx, body_len == 0 && cb == NULL);
    if (sid < 0) RETURN_FALSE;

    trace_emit("SEND_REQ", sid);

    /* body handling */
    if (body_len) {
        quiche_h3_send_body(s->h3, s->conn, sid, (const uint8_t *)body, body_len, cb == NULL);
    }

    if (cb) {
        /* streaming: repeatedly invoke callback to fetch chunks until it returns false */
        zval rv, params[1];
        ZVAL_LONG(&params[0], sid);
        while (1) {
            zval ret;
            if (call_user_function(EG(function_table), NULL, cb, &ret, 1, params) == FAILURE) break;
            if (Z_TYPE(ret) != IS_STRING || Z_STRLEN(ret) == 0) { zval_ptr_dtor(&ret); break; }
            quiche_h3_send_body(s->h3, s->conn, sid, (uint8_t *)Z_STRVAL(ret), Z_STRLEN(ret), 0);
            zval_ptr_dtor(&ret);
        }
        /* final FIN flag */
        quiche_h3_send_body(s->h3, s->conn, sid, NULL, 0, 1);
    }

    RETURN_LONG(sid);
}

/* ---------------------------------------------------------------
 * Header enumerator callback → PHP array
 * ---------------------------------------------------------------*/
static int hdr_cb(uint8_t *n, size_t nl, uint8_t *v, size_t vl, void *arg)
{
    add_assoc_stringl_ex((zval *)arg, (char *)n, nl, (char *)v, vl);
    return 0;
}

/* ---------------------------------------------------------------
 * quicpro_receive_response
 * ---------------------------------------------------------------*/
PHP_FUNCTION(quicpro_receive_response)
{
    zval *zsess; zend_long stream_id;
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "rl", &zsess, &stream_id) == FAILURE)
        RETURN_NULL();

    quicpro_session_t *s = zend_fetch_resource(Z_RES_P(zsess), "quicpro", le_quicpro);
    if (!s) RETURN_NULL();

    quiche_h3_event *ev;
    int rc = quiche_h3_conn_poll(s->h3, s->conn, &ev);
    if (rc < 0) RETURN_NULL();

#if QUICHE_H3_VERSION_ID >= 0x000a
    if (quiche_h3_event_stream_id(ev) != (uint64_t)stream_id) {
        quiche_h3_event_free(ev);
        RETURN_NULL();
    }
#endif

    int etype = quiche_h3_event_type(ev);
    if (etype == QUICHE_H3_EVENT_HEADERS) {
        zval hdr; array_init(&hdr);
        quiche_h3_event_for_each_header(ev, hdr_cb, &hdr);
        quiche_h3_event_free(ev);
        trace_emit("RECV_HDR", stream_id);
        RETURN_ZVAL(&hdr, 0, 1);
    }

    if (etype == QUICHE_H3_EVENT_DATA) {
        smart_str resp = {0};
        while (1) {
            uint8_t buf[8192];
            ssize_t n = quiche_h3_recv_body(s->h3, s->conn, stream_id, buf, sizeof(buf));
            if (n == QUICHE_H3_ERR_DONE) break; /* no more */
            if (n < 0) { quiche_h3_event_free(ev); RETURN_NULL(); }
            smart_str_appendl(&resp, (char *)buf, n);
        }
        quiche_h3_event_free(ev);
        smart_str_0(&resp);
        trace_emit("RECV_DATA", stream_id);
        RETVAL_STRINGL(ZSTR_VAL(resp.s), ZSTR_LEN(resp.s));
        smart_str_free(&resp);
        return;
    }

    quiche_h3_event_free(ev);
    RETURN_NULL();
}
