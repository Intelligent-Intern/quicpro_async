/* cancel.c – quicpro_cancel_stream() implementation
 *
 * This file implements the stream‐shutdown API for the quicpro_async extension.
 * When a PHP script calls quicpro_cancel_stream(), we translate that into
 * the appropriate quiche call to close either the read side, write side,
 * or both sides of a given QUIC stream. This is essential for proper
 * resource cleanup and half‐close semantics in HTTP/3. The code here
 * ensures that any protocol‐level error is converted into a PHP exception
 * so application code can catch and handle it. Below, we define two main
 * parts: helpers to map user requests into QUIC flags, and the public
 * PHP function exposed to scripts.
 */

#include <stddef.h>
#include <zend_exceptions.h>
#include "php_quicpro.h"
#include <quiche.h>
#include "session.h"
#include "cancel.h"

/*---------- helpers --------------------------------------------------------
 *
 * In this section we define internal utility functions that are not
 * directly exposed to PHP but are used by the public API. The primary helper
 * is `how_to_flags()`, which takes a string like "read" or "write" and
 * returns the corresponding quiche shutdown flags. We also provide
 * `throw_as_php_exception()`, which maps any quiche error code to one of
 * our PHP exception classes. This mapping is essential to present meaningful
 * errors to the user, instead of raw integers. We choose which PHP exception
 * subclass to use based on quiche’s negative return codes. Finally, we use
 * `zend_throw_exception_ex()` to raise an exception with a formatted message
 * that includes the numeric error code, so developers can log or debug issues
 * when a stream shutdown operation fails.
 */

static inline int how_to_flags(const char *how, size_t len)
{
    /* QUICHE_SHUTDOWN_READ  shuts down the read side of the stream. */
    if (len == 4 && strncasecmp(how, "read", 4) == 0)  return QUICHE_SHUTDOWN_READ;
    /* QUICHE_SHUTDOWN_WRITE shuts down the write side of the stream. */
    if (len == 5 && strncasecmp(how, "write",5) == 0)  return QUICHE_SHUTDOWN_WRITE;
    /* Default is to close both read and write if the argument is unrecognized. */
    return QUICHE_SHUTDOWN_READ | QUICHE_SHUTDOWN_WRITE;
}

static inline void throw_as_php_exception(int quiche_err)
{
    zend_class_entry *ce = quicpro_ce_exception;

    /* Map specific quiche error codes to our PHP exception subclasses. */
    switch (quiche_err) {
#define CASE_MAP(err_code, exc_class) \
        case err_code: ce = exc_class; break;

#ifdef QUICHE_ERR_INVALID_STREAM_STATE
        CASE_MAP(QUICHE_ERR_INVALID_STREAM_STATE,  quicpro_ce_invalid_state)
#endif
#ifdef QUICHE_ERR_UNKNOWN_STREAM
        CASE_MAP(QUICHE_ERR_UNKNOWN_STREAM,        quicpro_ce_unknown_stream)
#endif
#ifdef QUICHE_ERR_STREAM_BLOCKED
        CASE_MAP(QUICHE_ERR_STREAM_BLOCKED,        quicpro_ce_stream_blocked)
#endif
#ifdef QUICHE_ERR_STREAM_LIMIT
        CASE_MAP(QUICHE_ERR_STREAM_LIMIT,          quicpro_ce_stream_limit)
#endif
#ifdef QUICHE_ERR_FINAL_SIZE
        CASE_MAP(QUICHE_ERR_FINAL_SIZE,            quicpro_ce_final_size)
#endif
#ifdef QUICHE_ERR_STREAM_STOPPED
        CASE_MAP(QUICHE_ERR_STREAM_STOPPED,        quicpro_ce_stream_stopped)
#endif
#ifdef QUICHE_ERR_FIN_EXPECTED
        CASE_MAP(QUICHE_ERR_FIN_EXPECTED,          quicpro_ce_fin_expected)
#endif
#ifdef QUICHE_ERR_INVALID_FIN_STATE
        CASE_MAP(QUICHE_ERR_INVALID_FIN_STATE,     quicpro_ce_invalid_fin_state)
#endif
#ifdef QUICHE_ERR_DONE
        CASE_MAP(QUICHE_ERR_DONE,                  quicpro_ce_done)
#endif
#ifdef QUICHE_ERR_CONGESTION_CONTROL
        CASE_MAP(QUICHE_ERR_CONGESTION_CONTROL,    quicpro_ce_congestion_control)
#endif
#ifdef QUICHE_ERR_TOO_MANY_OPEN_STREAMS
        CASE_MAP(QUICHE_ERR_TOO_MANY_OPEN_STREAMS, quicpro_ce_too_many_streams)
#endif

#undef CASE_MAP
    }

    /* Finally throw: include the numeric code in the message for clarity. */
    zend_throw_exception_ex(
        ce,
        quiche_err,
        "QUIC stream error %d",
        quiche_err
    );
}

/*---------- userland API ---------------------------------------------------
 *
 * quicpro_cancel_stream() is the function PHP code calls to perform a
 * half-close or full-close on a specific HTTP/3 stream. The user passes
 * the session resource, the stream ID, and optionally "read" or "write"
 * to indicate which direction to shut down. We parse these parameters,
 * look up the underlying quicpro_session_t, and then call into quiche:
 * quiche_conn_stream_shutdown(). If quiche returns a non‐zero error,
 * we convert it via throw_as_php_exception() and return false to PHP.
 * On success, we return true. This lets PHP applications gracefully
 * clean up streams and trigger end‐of‐stream notifications on the peer.
 */

PHP_FUNCTION(quicpro_cancel_stream)
{
    zval      *z_sess;
    zend_long  stream_id;
    char      *how      = "both";
    size_t     how_len  = 4;

    ZEND_PARSE_PARAMETERS_START(2, 3)
        Z_PARAM_RESOURCE(z_sess)
        Z_PARAM_LONG(stream_id)
        Z_PARAM_OPTIONAL
        Z_PARAM_STRING(how, how_len)
    ZEND_PARSE_PARAMETERS_END();

    /* Fetch our internal session object from the PHP resource. */
    quicpro_session_t *s = zend_fetch_resource(
        Z_RES_P(z_sess), "quicpro", le_quicpro_session
    );
    if (!s) {
        /* If fetching fails, throw and bail. */
        RETURN_THROWS();
    }

    /* Invoke quiche to shut down the specified stream direction. */
    int rc = quiche_conn_stream_shutdown(
        s->conn,
        (uint64_t)stream_id,
        how_to_flags(how, how_len),
        0
    );

    /* If quiche returns an error, convert to a PHP exception. */
    if (rc) {
        throw_as_php_exception(rc);
        RETURN_FALSE;
    }

    /* On success, return TRUE to the caller. */
    RETURN_TRUE;
}
