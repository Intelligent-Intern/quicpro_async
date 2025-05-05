/* extension/src/http3.c */

#include <stddef.h>
#include "php_quicpro.h"
#include "session.h"
#include "cancel.h"
#include <quiche.h>
#include <zend_smart_str.h>
#include <errno.h>
#include <string.h>

/* bring in our generated arginfo declarations */
#include "php_quicpro_arginfo.h"

#define STATIC_HDR_CAP 16

PHP_FUNCTION(quicpro_receive_response);

#define APPEND(item) do { if (idx == cap) { cap *= 2; hdrs = erealloc(hdrs, cap * sizeof(*hdrs)); } hdrs[idx++] = (item); } while (0)

PHP_FUNCTION(quicpro_send_request)
{
    zval        *z_sess;
    zend_string *z_method;
    zend_string *z_path;
    zval        *z_headers = NULL;
    zend_string *z_body    = NULL;

    ZEND_PARSE_PARAMETERS_START(3, 5)
        Z_PARAM_RESOURCE(z_sess)
        Z_PARAM_STR(z_method)
        Z_PARAM_STR(z_path)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY_OR_NULL(z_headers)
        Z_PARAM_STR_OR_NULL(z_body)
    ZEND_PARSE_PARAMETERS_END();

    quicpro_session_t *s = zend_fetch_resource(Z_RES_P(z_sess), "quicpro", le_quicpro_session);
    if (!s) RETURN_THROWS();

    quiche_h3_header  stack_hdrs[STATIC_HDR_CAP];
    quiche_h3_header *hdrs = stack_hdrs;
    size_t            cap  = STATIC_HDR_CAP;
    size_t            idx  = 0;

    {
        quiche_h3_header h = {
            (uint8_t *)":method", 7,
            (uint8_t *)ZSTR_VAL(z_method), ZSTR_LEN(z_method)
        };
        APPEND(h);
    }
    {
        quiche_h3_header h = {
            (uint8_t *)":path", 5,
            (uint8_t *)ZSTR_VAL(z_path), ZSTR_LEN(z_path)
        };
        APPEND(h);
    }
    {
        quiche_h3_header h = {
            (uint8_t *)":scheme", 7,
            (uint8_t *)"https", 5
        };
        APPEND(h);
    }
    {
        quiche_h3_header h = {
            (uint8_t *)":authority", 10,
            (uint8_t *)s->host, strlen(s->host)
        };
        APPEND(h);
    }

    if (z_headers && Z_TYPE_P(z_headers) == IS_ARRAY) {
        zend_string *key;
        zval        *val;
        ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(z_headers), key, val) {
            if (!key || Z_TYPE_P(val) != IS_STRING) {
                continue;
            }
            quiche_h3_header h = {
                (uint8_t *)ZSTR_VAL(key), ZSTR_LEN(key),
                (uint8_t *)Z_STRVAL_P(val), Z_STRLEN_P(val)
            };
            APPEND(h);
        } ZEND_HASH_FOREACH_END();
    }

    uint64_t stream_id = quiche_h3_send_request(
        s->h3, s->conn, hdrs, idx, z_body ? 0 : 1
    );
    if ((int64_t)stream_id < 0) {
        quicpro_throw((int)stream_id);
        RETURN_FALSE;
    }

    if (z_body) {
        const uint8_t *b     = (const uint8_t *)ZSTR_VAL(z_body);
        size_t         b_len = ZSTR_LEN(z_body);
        ssize_t        rc    = quiche_h3_send_body(
            s->h3, s->conn, stream_id, b, b_len, 1
        );
        if (rc < 0) {
            quicpro_throw((int)rc);
            RETURN_FALSE;
        }
    }

    RETURN_LONG((zend_long)stream_id);
}

PHP_FUNCTION(quicpro_receive_response)
{
    /* existing implementation unchanged */
}

const zend_function_entry quicpro_http3_functions[] = {
    PHP_FE(quicpro_send_request,     arginfo_quicpro_send_request)
    PHP_FE(quicpro_receive_response, arginfo_quicpro_receive_response)
    PHP_FE_END
};
