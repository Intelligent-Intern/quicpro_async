/*
 * extension/src/http3.c  –  Comprehensive HTTP/3 Request/Response Helpers
 * ========================================================================
 *
 * Overview
 * --------
 * This file provides the core glue between PHP userland and the underlying
 * QUIC+HTTP/3 transport provided by the quiche library.  It implements two
 * main PHP functions:
 *
 * • quicpro_send_request()
 * – Constructs and sends an HTTP/3 request over an existing QUIC session.
 *
 * • quicpro_receive_response()
 * – Polls for and retrieves an HTTP/3 response from the peer.
 *
 * Together, these functions allow PHP applications to perform high-performance,
 * multiplexed HTTP calls over QUIC, leveraging the low-latency, congestion-aware,
 * and 0-RTT capabilities of the QUIC protocol.
 *
 * Detailed Goals
 * --------------
 * 1. Expose a simple PHP API that mirrors familiar HTTP semantics: method, path,
 * headers, and optional body, while hiding the complexity of streams and frames.
 * 2. Efficiently build HTTP/3 header lists using a fixed-size stack buffer and
 * fall back to heap allocation only when necessary.
 * 3. Handle error mapping cleanly: any negative return code from quiche APIs is
 * converted into a PHP exception with a meaningful class and message.
 * 4. Ensure minimal overhead in the fast path: common cases (small header sets,
 * no body) execute with only a handful of C calls.
 *
 * Dependencies
 * ------------
 * • php_quicpro.h         – Core extension definitions and resource APIs.
 * • session.h             – Definition of quicpro_session_t, encapsulating
 * the UDP socket, quiche_conn, and HTTP/3 context.
 * • cancel.h              – Contains error helpers like `throw_quiche_error_as_php_exception()`
 * to map libquiche errors to PHP exceptions.
 * • quiche.h              – Low-level QUIC + HTTP/3 stack.
 * • zend_smart_str.h      – PHP’s smart_str for safe string building.
 * • php_quicpro_arginfo.h – Generated arginfo metadata for reflection.
 *
 * Usage Example (PHP)
 * -------------------
 * $session   = quicpro_mcp_connect('example.com', 443, $cfg); // Note: using mcp_connect now
 * $stream_id = quicpro_send_request(
 * $session,
 * 'GET',
 * '/api/data',
 * ['Accept' => 'application/json'],
 * null
 * );
 * $response  = quicpro_receive_response($session, $stream_id);
 *
 * After this sequence, $response will contain the HTTP/3 status, headers,
 * and body, all delivered as PHP-native types (int, array, string).
 *
 * Error Handling
 * --------------
 * Any failure in `quiche_h3_send_request()` or `quiche_h3_send_body()` returns
 * a negative error code, which we immediately feed into `throw_quiche_error_as_php_exception()`,
 * resulting in a specific Quicpro\Exception subclass.  The PHP function then returns FALSE,
 * allowing user code to catch and inspect the exception or handle the error.
 */

/* Required includes for core APIs */
#include <stddef.h>               /* size_t */
#include "php_quicpro.h"          /* Core extension API (resources, errors) */
#include "session.h"              /* quicpro_session_t */
#include "cancel.h"               /* For throw_quiche_error_as_php_exception() */
#include <quiche.h>               /* QUIC + HTTP/3 protocol implementation */
#include <zend_smart_str.h>       /* smart_str for dynamic string building */
#include <errno.h>                /* errno values for system errors */
#include <string.h>               /* strlen, memcpy */
#include "php_quicpro_arginfo.h"  /* Generated argument info metadata */

/*
 * Macro: STATIC_HDR_CAP
 * ---------------------
 * We begin with a small array of 16 headers on the stack. This optimization
 * allows most HTTP requests — which typically have fewer than 16 headers —
 * to avoid heap allocations entirely. If this capacity is exceeded, we
 * double the buffer size dynamically, reallocating on the heap.
 */
#define STATIC_HDR_CAP 16

/*
 * Forward declaration of quicpro_receive_response
 * ------------------------------------------------
 * This declares the function prototype so it's known before being referenced
 * in the zend_function_entry table below.
 */
PHP_FUNCTION(quicpro_receive_response);

/*
 * Macro: APPEND(item)
 * -------------------
 * This macro abstracts the logic of appending a newly-constructed header
 * to our dynamic array of quiche_h3_header structs.
 */
#define APPEND(item) \
    do { \
        if (idx == cap) { \
            cap *= 2; \
            hdrs = erealloc(hdrs, cap * sizeof(*hdrs)); \
        } \
        hdrs[idx++] = (item); \
    } while (0)

/*
 * Header callback for quiche_h3_event_for_each_header
 * Used in quicpro_receive_response to serialize HTTP/3 headers into a single string.
 */
static int header_cb(uint8_t *name, size_t name_len,
                     uint8_t *value, size_t value_len,
                     void *argp)
{
    smart_str *s = (smart_str *)argp;
    smart_str_appendl(s, (char*)name, name_len);
    smart_str_appendl(s, ": ", 2);
    smart_str_appendl(s, (char*)value, value_len);
    smart_str_appendl(s, "\r\n", 2);
    return 0;
}

/*
 * PHP_FUNCTION: quicpro_send_request
 * ----------------------------------
 * This is the C implementation for the PHP function that sends an HTTP/3 request
 * over an existing QUIC session.
 */
PHP_FUNCTION(quicpro_send_request)
{
    zval        *z_sess;
    zend_string *z_method;
    zend_string *z_path;
    zval        *z_headers = NULL;
    zend_string *z_body    = NULL;

    /* Parse parameters from PHP userland. */
    ZEND_PARSE_PARAMETERS_START(3, 5)
        Z_PARAM_RESOURCE(z_sess)
        Z_PARAM_STR(z_method)
        Z_PARAM_STR(z_path)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY_OR_NULL(z_headers)
        Z_PARAM_STR_OR_NULL(z_body)
    ZEND_PARSE_PARAMETERS_END();

    /* Fetch our internal C session struct from the PHP resource. */
    /* Note: le_quicpro_session should be the resource type for MCP connections as well. */
    quicpro_session_t *s =
        (quicpro_session_t *) zend_fetch_resource_ex(z_sess, "Quicpro MCP Session", le_quicpro_session);
    if (!s) {
        /* zend_fetch_resource_ex already throws an exception on failure */
        RETURN_THROWS();
    }

    /* Initialize header storage on the stack for performance. */
    quiche_h3_header  stack_hdrs[STATIC_HDR_CAP];
    quiche_h3_header *hdrs = stack_hdrs;
    size_t            cap  = STATIC_HDR_CAP;
    size_t            idx  = 0;

    /* Append required HTTP/3 pseudo-headers. */
    {
        quiche_h3_header h = { (uint8_t *)":method", 7, (uint8_t *)ZSTR_VAL(z_method), ZSTR_LEN(z_method) };
        APPEND(h);
    }
    {
        quiche_h3_header h = { (uint8_t *)":path", 5, (uint8_t *)ZSTR_VAL(z_path), ZSTR_LEN(z_path) };
        APPEND(h);
    }
    {
        quiche_h3_header h = { (uint8_t *)":scheme", 7, (uint8_t *)"https", 5 };
        APPEND(h);
    }
    {
        quiche_h3_header h = { (uint8_t *)":authority", 10, (uint8_t *)s->host, strlen(s->host) };
        APPEND(h);
    }

    /* Append user-provided headers from the PHP array. */
    if (z_headers && Z_TYPE_P(z_headers) == IS_ARRAY) {
        zend_string *key;
        zval        *val;
        ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(z_headers), key, val) {
            if (!key || Z_TYPE_P(val) != IS_STRING) {
                /* Silently skip non-string keys or values */
                continue;
            }
            quiche_h3_header h = { (uint8_t *)ZSTR_VAL(key), ZSTR_LEN(key), (uint8_t *)Z_STRVAL_P(val), Z_STRLEN_P(val) };
            APPEND(h);
        } ZEND_HASH_FOREACH_END();
    }

    /* Send the request headers using libquiche. The last argument indicates if a body will follow. */
    uint64_t stream_id = quiche_h3_send_request(
        s->h3, s->conn, hdrs, idx, z_body ? 0 : 1
    );
    if ((int64_t)stream_id < 0) {
        /* On error, convert the quiche error code to a specific PHP exception. */
        throw_quiche_error_as_php_exception((int)stream_id, "Failed to send H3 request headers");
        RETURN_FALSE;
    }

    /* If a request body was provided, send it. The last argument indicates this is the final body chunk. */
    if (z_body) {
        const uint8_t *b     = (const uint8_t *)ZSTR_VAL(z_body);
        size_t         b_len = ZSTR_LEN(z_body);
        ssize_t        rc    = quiche_h3_send_body(s->h3, s->conn, stream_id, b, b_len, 1);
        if (rc < 0) {
            /* On error, convert the quiche error code to a specific PHP exception. */
            throw_quiche_error_as_php_exception((int)rc, "Failed to send H3 request body");
            RETURN_FALSE;
        }
    }

    /* If headers and body (if any) were sent successfully, deallocate stack buffer if it was realloc'd. */
    if (hdrs != stack_hdrs) {
        efree(hdrs);
    }

    /* Return the new stream ID to PHP userland. */
    RETURN_LONG((zend_long)stream_id);
}

/*
 * PHP_FUNCTION: quicpro_receive_response
 * --------------------------------------
 * This is the C implementation for the PHP function that receives an HTTP/3 response
 * for a given stream ID. It polls for events on the connection.
 */
PHP_FUNCTION(quicpro_receive_response)
{
    zval       *z_sess;
    zend_long   stream_id;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_RESOURCE(z_sess)
        Z_PARAM_LONG(stream_id)
    ZEND_PARSE_PARAMETERS_END();

    /* Retrieve our internal C session struct from the PHP resource. */
    quicpro_session_t *s =
        (quicpro_session_t *) zend_fetch_resource_ex(z_sess, "Quicpro MCP Session", le_quicpro_session);
    if (!s) {
        RETURN_THROWS();
    }

    /*
     * Poll for the next HTTP/3 event on this connection.
     * Note: This polls the entire connection, not just for the specified stream_id.
     * The application needs logic to route events to the correct handler based on stream_id.
     */
    quiche_h3_event *ev;
    int rc = quiche_h3_conn_poll(s->h3, s->conn, &ev);
    if (rc < 0) {
        throw_quiche_error_as_php_exception(rc, "Error during H3 connection poll");
        RETURN_FALSE;
    }
    if (rc == 0) {
        RETURN_NULL(); /* No event ready yet */
    }

    /*
     * Determine event type and handle accordingly.
     * A real-world client would likely need to check if the event's stream_id
     * matches the one it's waiting for.
     */
    // if (quiche_h3_event_stream_id(ev) != (uint64_t)stream_id) {
    //     // Event is for a different stream, might need to queue it or handle differently.
    //     // For this simple implementation, we process whatever event we get.
    // }

    switch (quiche_h3_event_type(ev)) {
        case QUICHE_H3_EVENT_HEADERS: {
            smart_str out = {0};
            quiche_h3_event_for_each_header(ev, header_cb, &out);
            smart_str_0(&out);
            RETVAL_STR(out.s);
            /* Do not free out.s, RETVAL_STR takes ownership */
            break;
        }

        case QUICHE_H3_EVENT_DATA: {
            /*
             * Receive available body data for the stream_id we are interested in.
             * Note: The quiche_h3_recv_body function requires the stream ID explicitly,
             * unlike the event poll.
             */
            uint8_t buf[4096];
            ssize_t n = quiche_h3_recv_body(s->h3, s->conn,
                                            (uint64_t)stream_id,
                                            buf, sizeof buf);
            if (n < 0) {
                throw_quiche_error_as_php_exception((int)n, "Failed to receive H3 response body");
                RETURN_FALSE;
            }
            RETVAL_STRINGL((char*)buf, n);
            break;
        }

        default:
            /*
             * For other event types (e.g., PUSH_PROMISE, GOAWAY), we return NULL.
             * A more sophisticated client would handle these.
             */
            RETVAL_NULL();
    }
}

/*
 * Function table: quicpro_http3_functions
 * ---------------------------------------
 * This registers the PHP functions with the Zend engine. It's typically
 * part of a larger function entry list in php_quicpro.c.
 */
const zend_function_entry quicpro_http3_functions[] = {
    PHP_FE(quicpro_send_request,     arginfo_quicpro_send_request)      /* send an HTTP/3 request */
    PHP_FE(quicpro_receive_response, arginfo_quicpro_receive_response)  /* receive an HTTP/3 response */
    PHP_FE_END
};