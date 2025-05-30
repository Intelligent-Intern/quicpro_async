/* ------------------------------------------------------------------------- */
/* cancel.c – Implementation of error mapping and stream shutdown helpers     */
/*                                                                            */
/* This file provides the implementation for:                                 */
/*   - how_to_flags: Converts userland string into quiche shutdown bitmask    */
/*   - throw_as_php_exception: Converts error code to PHP exception           */
/*   - quicpro_cancel_stream: PHP API for stream close/half-close             */
/*                                                                            */
/* It is tightly integrated with the exception classes defined in php_quicpro.h*/
/* and the resource types (session).                                          */
/* ------------------------------------------------------------------------- */

#include "php.h"
#include <zend_exceptions.h>
#include "php_quicpro.h"
#include "session.h"
#include "cancel.h"
#include <quiche.h>
#include <stddef.h>

/*
 * how_to_flags
 * ------------
 * Internal helper that translates the PHP user string into the correct
 * quiche shutdown flags.
 */
int how_to_flags(const char *how, size_t len)
{
    /* Match "read" exactly (case-insensitive) */
    if (len == 4 && strncasecmp(how, "read", 4) == 0)  return QUICHE_SHUTDOWN_READ;
    /* Match "write" exactly (case-insensitive) */
    if (len == 5 && strncasecmp(how, "write",5) == 0)  return QUICHE_SHUTDOWN_WRITE;
    /* Default: close both directions if unknown or "both" */
    return QUICHE_SHUTDOWN_READ | QUICHE_SHUTDOWN_WRITE;
}

/*
 * throw_as_php_exception
 * ---------------------
 * Maps specific quiche error codes to well-typed PHP exception classes.
 * Uses a macro for maintainability and to reduce copy-paste.
 * If a known error code is matched, throws the specific subclass;
 * otherwise falls back to quicpro_ce_exception (the base class).
 */
void throw_as_php_exception(int quiche_err)
{
    zend_class_entry *ce = quicpro_ce_exception;

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

    /* Always throw a PHP exception, include the error code for context */
    zend_throw_exception_ex(
        ce,
        quiche_err,
        "QUIC stream error %d",
        quiche_err
    );
}

/*
 * quicpro_cancel_stream
 * --------------------
 * PHP API entry point for closing/half-closing a stream.
 * Receives the session resource, the stream ID, and an optional mode string.
 * Delegates to how_to_flags() for shutdown mode, and quiche_conn_stream_shutdown
 * for protocol execution.
 * Returns true on success, false and throws on protocol error.
 */
PHP_FUNCTION(quicpro_cancel_stream)
{
    zval      *z_sess;
    zend_long  stream_id;
    char      *how      = "both";
    size_t     how_len  = 4;

    /* Parse parameters: resource, stream ID, optional shutdown mode */
    ZEND_PARSE_PARAMETERS_START(2, 3)
        Z_PARAM_RESOURCE(z_sess)
        Z_PARAM_LONG(stream_id)
        Z_PARAM_OPTIONAL
        Z_PARAM_STRING(how, how_len)
    ZEND_PARSE_PARAMETERS_END();

    /* Fetch the internal session object from the PHP resource. */
    quicpro_session_t *s = zend_fetch_resource(
        Z_RES_P(z_sess), "quicpro", le_quicpro_session
    );
    if (!s) {
        RETURN_THROWS();
    }

    /* Perform the shutdown operation using the mapped flags. */
    int rc = quiche_conn_stream_shutdown(
        s->conn,
        (uint64_t)stream_id,
        how_to_flags(how, how_len),
        0
    );

    /* On error: throw an exception and return false to userland. */
    if (rc) {
        throw_as_php_exception(rc);
        RETURN_FALSE;
    }

    /* On success: return true. */
    RETURN_TRUE;
}
