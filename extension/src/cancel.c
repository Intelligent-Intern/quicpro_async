/* cancel.c – quicpro_cancel_stream() implementation
 * --------------------------------------------------*/

#include <stddef.h>
#include <zend_exceptions.h>
#include "php_quicpro.h"
#include <quiche.h>
#include "session.h"
#include "cancel.h"

/*---------- helpers --------------------------------------------------------*/

static inline int how_to_flags(const char *how, size_t len)
{
    if (len == 4 && strncasecmp(how, "read", 4) == 0)  return QUICHE_SHUTDOWN_READ;
    if (len == 5 && strncasecmp(how, "write",5) == 0)  return QUICHE_SHUTDOWN_WRITE;
    return QUICHE_SHUTDOWN_READ | QUICHE_SHUTDOWN_WRITE;
}

static inline void throw_as_php_exception(int quiche_err)
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

    /* Throw with a generic QUIC-stream error message */
    zend_throw_exception_ex(
        ce,
        quiche_err,
        "QUIC stream error %d",
        quiche_err
    );
}

/*---------- userland API ---------------------------------------------------*/

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

    quicpro_session_t *s = zend_fetch_resource(Z_RES_P(z_sess), "quicpro", le_quicpro_session);
    if (!s) {
        RETURN_THROWS();
    }

    int rc = quiche_conn_stream_shutdown(
        s->conn, (uint64_t)stream_id, how_to_flags(how, how_len), 0
    );

    if (rc) {
        throw_as_php_exception(rc);
        RETURN_FALSE;
    }

    RETURN_TRUE;
}
