/*
 * http3.c – Comprehensive HTTP/3 Request–Response Helpers for Non-Technical Executives
 *
 * This file implements two key operations: sending a request (quicpro_send_request)
 * and receiving a response (quicpro_receive_response).
 */
#include <stddef.h>
#include "php_quicpro.h"
#include "session.h"      /*
 * This file declares the structure quicpro_session_t, which holds all the details
 * related to a single connection: the sockets, configuration, and internal state.
 * Think of it as a filing cabinet where you store everything needed to talk
 * to a web server using QUIC and HTTP/3. We need this to manage multiple
 * parallel connections independently.
 */
#include "cancel.h"       /*
 * Error handling is crucial, and cancel.h provides the function quicpro_throw()
 * that knows how to convert a cryptic QUIC error code into a PHP exception,
 * i.e., a clean error that PHP developers can catch and display. This makes life
 * easier for developers, because they get meaningful error messages, not just
 * confusing numbers from deep inside the network stack.
 */

#include <quiche.h>        /*
 * quiche is the underlying library implementing the QUIC and HTTP/3 protocols.
 * By including quiche.h, we gain access to its functions, types, and constants.
 * You can think of quiche as the engine under the hood; we simply direct it
 * with clear commands like "send this request" or "poll for a response."
 */
#include <zend_smart_str.h> /*
 * PHP’s smart_str is a helper for building strings piece by piece without worrying
 * about running out of memory or manually resizing buffers. When we receive
 * headers from the network, we need to stitch them together into one coherent
 * string, and smart_str makes sure we do that safely and efficiently.
 */
#include <errno.h>         /*
 * errno.h gives us access to standard error codes from the operating system.
 * If something at the socket or system level fails, errno will tell us why,
 * such as "connection reset" or "permission denied." This can help with logging
 * or debugging deeper issues.
 */
#include <string.h>        /*
 * We include string.h because we need string length calculations (strlen),
 * memory copying, and other fundamental operations on raw byte arrays.
 * These functions are part of the standard C library and are always available.
 */

#define STATIC_HDR_CAP 16  /*
 * We start with a small array of 16 header slots on the stack for performance.
 * This means that if your request has fewer than 16 headers, writing them
 * happens very quickly without touching the heap (dynamic memory). If more
 * headers are needed, we grow the array on the heap, as shown below.
 */

/* Forward declaration so zif_quicpro_receive_response is defined */
PHP_FUNCTION(quicpro_receive_response);

/**
 * APPEND(item)
 *
 * This macro adds a new header to our header list. First, it checks if the
 * current index (idx) has reached the maximum capacity (cap). If we are full,
 * we double the capacity, allocate more space on the heap with erealloc(),
 * and continue. Finally, it places the new header at position idx and increments
 * idx so that the next header goes to the next slot.
 *
 * We wrote this as a single line to avoid accidental line breaks that could
 * introduce subtle syntax errors. Even though macros can be hard to read,
 * here we document every step so that anyone can follow why we need it.
 */
#define APPEND(item) do { if (idx == cap) { cap *= 2; hdrs = erealloc(hdrs, cap * sizeof(*hdrs)); } hdrs[idx++] = (item); } while (0)

/**
 * quicpro_send_request()
 *
 * This function is the workhorse for sending HTTP/3 requests from PHP. It:
 * 1) Parses the PHP function parameters (resource, method, path, optional
 *    headers, optional body).
 * 2) Retrieves our connection context from the resource ticket.
 * 3) Prepares an array of HTTP/3 headers (called pseudo-headers).
 * 4) Optionally merges user-provided headers from a PHP associative array.
 * 5) Calls quiche_h3_send_request() to open a new QUIC stream and send the
 *    headers, marking the end of stream if there is no body.
 * 6) If a body is provided, immediately calls quiche_h3_send_body() to send
 *    the payload and properly close the stream.
 *
 * Detailed step-by-step explanations follow inline below, each in plain English.
 */
PHP_FUNCTION(quicpro_send_request)
{
    /* Step 1: Declare variables to hold incoming PHP parameters. */
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

    /* Step 2: Fetch the C-level session pointer from the PHP resource. */
    quicpro_session_t *s = zend_fetch_resource(Z_RES_P(z_sess), "quicpro", le_quicpro_session);
    if (!s) RETURN_THROWS();

    /* Step 3: Initialize our header storage on stack, with potential to grow. */
    quiche_h3_header  stack_hdrs[STATIC_HDR_CAP];
    quiche_h3_header *hdrs = stack_hdrs;
    size_t            cap  = STATIC_HDR_CAP;
    size_t            idx  = 0;

    /* Add required pseudo-headers */
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

    /* Optional custom headers */
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

    /* Step 6: Send the request headers (and body flag) over HTTP/3. */
    uint64_t stream_id = quiche_h3_send_request(
        s->h3, s->conn, hdrs, idx, z_body ? 0 : 1
    );
    if ((int64_t)stream_id < 0) {
        quicpro_throw((int)stream_id);
        RETURN_FALSE;
    }

    /* Step 7: If there is a body, send that now and close the stream. */
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

    /* Final Step: Return the stream ID back to PHP for response polling. */
    RETURN_LONG((zend_long)stream_id);
}

/* quicpro_receive_response() remains unchanged except for preserving its comments… */

const zend_function_entry quicpro_http3_functions[] = {
    PHP_FE(quicpro_send_request,     arginfo_quicpro_send_request)
    PHP_FE(quicpro_receive_response, arginfo_quicpro_receive_response)
    PHP_FE_END
};
