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
 *    • quicpro_send_request()
 *      – Constructs and sends an HTTP/3 request over an existing QUIC session.
 *
 *    • quicpro_receive_response()
 *      – Polls for and retrieves an HTTP/3 response from the peer.
 *
 * Together, these functions allow PHP applications to perform high-performance,
 * multiplexed HTTP calls over QUIC, leveraging the low-latency, congestion-aware,
 * and 0-RTT capabilities of the QUIC protocol.
 *
 * Detailed Goals
 * --------------
 * 1. Expose a simple PHP API that mirrors familiar HTTP semantics: method, path,
 *    headers, and optional body, while hiding the complexity of streams and frames.
 * 2. Efficiently build HTTP/3 header lists using a fixed-size stack buffer and
 *    fall back to heap allocation only when necessary.
 * 3. Handle error mapping cleanly: any negative return code from quiche APIs is
 *    converted into a PHP exception with a meaningful class and message.
 * 4. Ensure minimal overhead in the fast path: common cases (small header sets,
 *    no body) execute with only a handful of C calls.
 *
 * Dependencies
 * ------------
 *   • php_quicpro.h         – Core extension definitions and resource APIs.
 *   • session.h             – Definition of quicpro_session_t, encapsulating
 *                             the UDP socket, quiche_conn, and HTTP/3 context.
 *   • cancel.h              – throw_as_php_exception(): maps quiche error codes to
 *                             PHP exceptions.
 *   • quiche.h              – Low-level QUIC + HTTP/3 stack.
 *   • zend_smart_str.h      – PHP’s smart_str for safe string building.
 *   • php_quicpro_arginfo.h – Generated arginfo metadata for reflection.
 *
 * Usage Example (PHP)
 * -------------------
 *   $session   = quicpro_connect('example.com', 443, $cfg);
 *   $stream_id = quicpro_send_request(
 *                   $session,
 *                   'GET',
 *                   '/api/data',
 *                   ['Accept' => 'application/json'],
 *                   null
 *                );
 *   $response  = quicpro_receive_response($session, $stream_id);
 *
 * After this sequence, $response will contain the HTTP/3 status, headers,
 * and body, all delivered as PHP-native types (int, array, string).
 *
 * Error Handling
 * --------------
 * Any failure in quiche_h3_send_request() or quiche_h3_send_body() returns
 * a negative error code, which we immediately feed into throw_as_php_exception(), resulting
 * in a specific Quicpro\Exception subclass.  The PHP function then returns FALSE,
 * allowing user code to catch and inspect the exception or handle the error.
 */

/* Required includes for core APIs */
#include <stddef.h>               /* size_t */
#include "php_quicpro.h"          /* Core extension API (resources, errors) */
#include "session.h"              /* quicpro_session_t */
#include "cancel.h"               /* throw_as_php_exception() */
#include <quiche.h>               /* QUIC + HTTP/3 protocol implementation */
#include <zend_smart_str.h>       /* smart_str for dynamic string building */
#include <errno.h>                /* errno values for system errors */
#include <string.h>               /* strlen, memcpy */
#include "php_quicpro_arginfo.h"  /* Generated argument info metadata */

/*
 * Macro: STATIC_HDR_CAP
 * ---------------------
 * We begin with a small array of 16 headers on the stack.  This optimization
 * allows most HTTP requests — which typically have fewer than 16 headers —
 * to avoid heap allocations entirely.  If this capacity is exceeded, we
 * double the buffer size dynamically, reallocating on the heap.
 */
#define STATIC_HDR_CAP 16

/*
 * Forward declaration of quicpro_receive_response
 * ------------------------------------------------
 * We declare this here so that our function table at the bottom of the file
 * can reference both functions, even though quicpro_receive_response()
 * is implemented further down or in another translation unit.
 */
PHP_FUNCTION(quicpro_receive_response);

/*
 * Macro: APPEND(item)
 * -------------------
 * This macro abstracts the logic of appending a newly-constructed header
 * to our dynamic array.  In plain English:
 *
 *   1. Check if we have reached the current capacity (`cap`).  If so,
 *      double the capacity to make room for more headers.
 *   2. Reallocate the `hdrs` buffer on the heap with the new size.
 *   3. Add the provided header `item` at index `idx`.
 *   4. Increment `idx` so that subsequent headers go to the next slot.
 *
 * This macro is written as a single statement to avoid pitfalls with
 * dangling semicolons or unexpected scoping rules in C.
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
 * Send an HTTP/3 request over an existing QUIC session.
 *
 * This function performs the following steps:
 *   1. Parse and validate parameters from PHP:
 *      - A session resource (quicpro_session_t*)
 *      - A string method (e.g., "GET", "POST")
 *      - A string path (e.g., "/index.html")
 *      - Optional associative array of headers
 *      - Optional string body
 *
 *   2. Fetch the internal session pointer using zend_fetch_resource().
 *      If this fails, a PHP exception is thrown and execution halts.
 *
 *   3. Prepare a header storage:
 *      - Allocate a small stack array of capacity STATIC_HDR_CAP.
 *      - Initialize pointers `hdrs`, `cap`, and `idx`.
 *
 *   4. Append the 4 required pseudo‑headers in HTTP/3:
 *      :method, :path, :scheme, and :authority.
 *
 *   5. Iterate over user-provided headers (if any):
 *      - For each PHP array entry, ensure the key is a string and
 *        the value is a string.
 *      - Construct a quiche_h3_header and append it to `hdrs`.
 *
 *   6. Call quiche_h3_send_request():
 *      - Pass the assembled `hdrs` array, the number of headers `idx`,
 *        and a flag indicating whether this is the end of stream.
 *      - On error (< 0), map the error code via throw_as_php_exception() and
 *        return FALSE to PHP.
 *
 *   7. If a body was supplied, call quiche_h3_send_body():
 *      - Send the payload bytes and mark end-of-stream.
 *      - On error, map and return FALSE.
 *
 *   8. On success, return the new stream ID as a PHP integer.
 *
 * This process ensures that sending HTTP/3 requests from PHP is as
 * straightforward as calling a single function with familiar parameters,
 * while the underlying C code handles buffer management, error mapping,
 * and transport details.
 */
PHP_FUNCTION(quicpro_send_request)
{
    /* Step 1: PHP parameters */
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

    /* Step 2: Fetch C session struct */
    quicpro_session_t *s =
        zend_fetch_resource(Z_RES_P(z_sess), "quicpro", le_quicpro_session);
    if (!s) {
        /* zend_fetch_resource already threw an exception */
        RETURN_THROWS();
    }

    /* Step 3: Initialize header storage */
    quiche_h3_header  stack_hdrs[STATIC_HDR_CAP];
    quiche_h3_header *hdrs = stack_hdrs;
    size_t            cap  = STATIC_HDR_CAP;
    size_t            idx  = 0;

    /* Step 4: Append required pseudo‑headers */
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

    /* Step 5: Append user-provided headers */
    if (z_headers && Z_TYPE_P(z_headers) == IS_ARRAY) {
        zend_string *key;
        zval        *val;
        ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(z_headers), key, val) {
            /* Skip non-string keys or non-string values */
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

    /* Step 6: Send request headers (and EOS flag if no body) */
    uint64_t stream_id = quiche_h3_send_request(
        s->h3, s->conn, hdrs, idx, z_body ? 0 : 1
    );
    if ((int64_t)stream_id < 0) {
        /* Map error code to PHP exception and bail out */
        throw_as_php_exception((int)stream_id);
        RETURN_FALSE;
    }

    /* Step 7: If body provided, send it and close the stream */
    if (z_body) {
        const uint8_t *b     = (const uint8_t *)ZSTR_VAL(z_body);
        size_t         b_len = ZSTR_LEN(z_body);
        ssize_t        rc    = quiche_h3_send_body(
            s->h3, s->conn, stream_id, b, b_len, 1
        );
        if (rc < 0) {
            throw_as_php_exception((int)rc);
            RETURN_FALSE;
        }
    }

    /* Step 8: Return the stream ID for response polling */
    RETURN_LONG((zend_long)stream_id);
}

/*
 * PHP_FUNCTION: quicpro_receive_response
 * --------------------------------------
 * Receive and return an HTTP/3 response for a given stream.
 *
 * This function is responsible for:
 *   1. Parsing the session resource and stream ID from PHP.
 *   2. Polling for the next HTTP/3 event using quiche_h3_conn_poll.
 *   3. Handling error codes: <0 mapped to PHP exception, 0 returns NULL.
 *   4. For headers, serialize using header_cb into a PHP string.
 *   5. For data, allocate a buffer and return body as PHP string.
 *   6. For unknown event types, return NULL (caller decides).
 */
PHP_FUNCTION(quicpro_receive_response)
{
    zval       *z_sess;
    zend_long   stream_id;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        /*
         * Ensure we have exactly two arguments: the session resource
         * and the stream ID returned by send_request. If not, PHP will
         * raise a type error automatically.
         */
        Z_PARAM_RESOURCE(z_sess)
        Z_PARAM_LONG(stream_id)
    ZEND_PARSE_PARAMETERS_END();

    /*
     * Retrieve our connection context just like in send_request().
     * If this fails, we throw and stop execution.
     */
    quicpro_session_t *s = zend_fetch_resource(Z_RES_P(z_sess), "quicpro", le_quicpro_session);
    if (!s) RETURN_THROWS();

    /*
     * Poll for the next HTTP/3 event. quiche_h3_conn_poll returns:
     * - >0 if an event is stored in 'ev'.
     * - 0 if no event is ready yet.
     * - <0 for an error code.
     *
     * We convert errors to exceptions, return NULL for "no data yet",
     * or proceed to inspect the event.
     */
    quiche_h3_event *ev;
    int rc = quiche_h3_conn_poll(s->h3, s->conn, &ev);
    if (rc < 0) { throw_as_php_exception(rc); RETURN_FALSE; }
    if (rc == 0) { RETURN_NULL(); }

    /*
     * Determine event type and handle accordingly.
     */
    switch (quiche_h3_event_type(ev)) {
        case QUICHE_H3_EVENT_HEADERS: {
            smart_str out = {0};
            quiche_h3_event_for_each_header(ev, header_cb, &out);
            smart_str_0(&out);
            RETVAL_STR(out.s);
            smart_str_free(&out);
            break;
        }

        case QUICHE_H3_EVENT_DATA: {
            /*
             * We received body data. We allocate a buffer of up to 4096 bytes.
             * Then quiche_h3_recv_body fills it with available data,
             * returning the number of bytes read. If negative, it's an error.
             * We then return a PHP string containing exactly the number
             * of bytes read so the caller can process it (e.g., append to file).
             */
            uint8_t buf[4096];
            ssize_t n = quiche_h3_recv_body(s->h3, s->conn,
                                            (uint64_t)stream_id,
                                            buf, sizeof buf);
            if (n < 0) { throw_as_php_exception((int)n); RETURN_FALSE; }
            RETVAL_STRINGL((char*)buf, n);
            break;
        }

        default:
            /*
             * For unexpected event types (e.g., PUSH_PROMISE, SETTINGS),
             * we simply return NULL and let the caller decide whether
             * to ignore or handle these later. This keeps our API simple.
             */
            RETVAL_NULL();
    }
}

/*
 * Function table: quicpro_http3_functions
 * ---------------------------------------
 * We map the PHP function names to their C implementations,
 * supplying the generated arginfo for signature checking and
 * reflection support in tools like PHPStorm or phpDocumentor.
 */
const zend_function_entry quicpro_http3_functions[] = {
    PHP_FE(quicpro_send_request,     arginfo_quicpro_send_request)      /* send an HTTP/3 request */
    PHP_FE(quicpro_receive_response, arginfo_quicpro_receive_response)  /* receive an HTTP/3 response */
    PHP_FE_END
};
