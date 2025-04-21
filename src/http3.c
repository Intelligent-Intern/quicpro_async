#include "php_quicpro.h"
#include <string.h>

#define HDR(n,v) (quiche_h3_header){(uint8_t *)n, sizeof(n)-1, (uint8_t *)v, sizeof(v)-1}

PHP_FUNCTION(quicpro_send_request)
{
    zval *zsess, *harr = NULL; char *path, *body = NULL;
    size_t path_len, body_len = 0;
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "rs|as",
                              &zsess, &path, &path_len,
                              &harr, &body, &body_len) == FAILURE)
        RETURN_FALSE;

    quicpro_session_t *s = zend_fetch_resource(Z_RES_P(zsess), "quicpro", le_quicpro);

    quiche_h3_header hdrs[64]; size_t idx = 0;
    hdrs[idx++] = HDR(":method","GET");
    hdrs[idx++] = HDR(":scheme","https");
    hdrs[idx++] = (quiche_h3_header){(uint8_t *)":path",5,(uint8_t *)path,path_len};
    hdrs[idx++] = HDR(":authority","localhost");

    if (harr && Z_TYPE_P(harr)==IS_ARRAY) {
        zend_string *k; zval *v;
        ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(harr), k, v) {
            if (!k || Z_TYPE_P(v)!=IS_STRING) continue;
            hdrs[idx++] = (quiche_h3_header){
                (uint8_t *)ZSTR_VAL(k), ZSTR_LEN(k),
                (uint8_t *)Z_STRVAL_P(v), Z_STRLEN_P(v)};
        } ZEND_HASH_FOREACH_END();
    }

    int64_t sid = quiche_h3_send_request(s->h3, s->conn,
                                         hdrs, idx,
                                         body_len == 0);
    if (sid < 0) RETURN_FALSE;

    if (body_len)
        quiche_h3_send_body(s->h3, s->conn, sid,
                            (const uint8_t *)body, body_len, true);

    RETURN_LONG(sid);
}

static int hdr_cb(uint8_t *n,size_t nl,uint8_t *v,size_t vl,void *arg)
{
    add_assoc_stringl_ex((zval *)arg,(char *)n,nl,(char *)v,vl);
    return 0;
}

PHP_FUNCTION(quicpro_receive_response)
{
    zval *zsess; zend_long stream_id;
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "rl", &zsess, &stream_id) == FAILURE)
        RETURN_NULL();

    quicpro_session_t *s = zend_fetch_resource(Z_RES_P(zsess), "quicpro", le_quicpro);

    quiche_h3_event *ev;
    if (quiche_h3_conn_poll(s->h3, s->conn, &ev) < 0) RETURN_NULL();

#if QUICHE_H3_VERSION_ID >= 0x000a  /* quiche ≥ 0.10 provides stream_id() */
    if (quiche_h3_event_stream_id(ev) != (uint64_t)stream_id) {
        quiche_h3_event_free(ev); RETURN_NULL();
    }
#endif

    if (quiche_h3_event_type(ev) == QUICHE_H3_EVENT_HEADERS) {
        zval hdr; array_init(&hdr);
        quiche_h3_event_for_each_header(ev, hdr_cb, &hdr);
        quiche_h3_event_free(ev);
        RETURN_ZVAL(&hdr, 0, 1);
    }

    if (quiche_h3_event_type(ev) == QUICHE_H3_EVENT_DATA) {
        uint8_t buf[8192];
        ssize_t n = quiche_h3_recv_body(s->h3, s->conn,
                                        stream_id, buf, sizeof(buf));
        quiche_h3_event_free(ev);
        if (n < 0) RETURN_NULL();
        RETVAL_STRINGL((char *)buf, n);
        return;
    }

    quiche_h3_event_free(ev);
    RETURN_NULL();
}