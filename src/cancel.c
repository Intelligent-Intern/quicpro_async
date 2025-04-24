/*  cancel.c  –  Stream shutdown helper for php‑quicpro
 *  ----------------------------------------------------
 *  Allows caller to cancel (reset) a stream in READ, WRITE or BOTH directions
 *  and maps quiche error codes to the PHP\Quicpro\Exception hierarchy.
 *
 *  User‑land signature:
 *      bool quicpro_cancel_stream(resource $sess,
 *                                 int      $stream_id,
 *                                 string   $how = "both")
 *
 *      $how ∈ { "read", "write", "both" }
 */

#include "php_quicpro.h"
#include <string.h> /* strncasecmp */

/* ------------------------------------------------------------ */
/* Helper: translate user‑land $how string → quiche shutdown mask */
/* ------------------------------------------------------------ */
static inline int quicpro_how2flags(const char *how, size_t len)
{
    if (len == 4 && strncasecmp(how, "read", 4) == 0)
        return QUICHE_SHUTDOWN_READ;
    if (len == 5 && strncasecmp(how, "write", 5) == 0)
        return QUICHE_SHUTDOWN_WRITE;
    return QUICHE_SHUTDOWN_READ | QUICHE_SHUTDOWN_WRITE; /* default: both */
}

/* ------------------------------------------------------------ */
/* Helper: throw the correct PHP exception for a quiche error    */
/* ------------------------------------------------------------ */
static inline void quicpro_throw(int err)
{
    zend_class_entry *ce = quicpro_ce_exception; /* base class, from php_quicpro.h */

    switch (err) {
        /* — Stream‑level errors — */
        case QUICHE_ERR_INVALID_STREAM_STATE:
            ce = quicpro_ce_invalid_state;       break;
        case QUICHE_ERR_UNKNOWN_STREAM:
            ce = quicpro_ce_unknown_stream;      break;
        case QUICHE_ERR_STREAM_BLOCKED:
            ce = quicpro_ce_stream_blocked;      break;
        case QUICHE_ERR_STREAM_LIMIT:
            ce = quicpro_ce_stream_limit;        break;
        case QUICHE_ERR_FINAL_SIZE:
            ce = quicpro_ce_final_size;          break;
        case QUICHE_ERR_STREAM_STOPPED:
            ce = quicpro_ce_stream_stopped;      break;
        case QUICHE_ERR_FIN_EXPECTED:
            ce = quicpro_ce_fin_expected;        break;
        case QUICHE_ERR_INVALID_FIN_STATE:
            ce = quicpro_ce_invalid_fin_state;   break;
        /* — Connection‑level / generic errors — */
        case QUICHE_ERR_DONE:
            ce = quicpro_ce_done;                break;
        case QUICHE_ERR_CONGESTION_CONTROL:
            ce = quicpro_ce_congestion_control;  break;
        case QUICHE_ERR_TOO_MANY_OPEN_STREAMS:
            ce = quicpro_ce_too_many_streams;    break;
        default:
            /* keep base class */
            break;
    }

    zend_throw_exception(ce, quicpro_err2str(err), err);
}

/* ------------------------------------------------------------ */
/* PHP function: quicpro_cancel_stream                         */
/* ------------------------------------------------------------ */
PHP_FUNCTION(quicpro_cancel_stream)
{
    zval      *zsess;
    zend_long  stream_id;
    char      *how      = "both";
    size_t     how_len  = 4;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "rl|s",
                              &zsess, &stream_id,
                              &how, &how_len) == FAILURE) {
        RETURN_THROWS();
    }

    quicpro_session_t *s = zend_fetch_resource(Z_RES_P(zsess), "quicpro", le_quicpro);
    if (!s) {
        RETURN_THROWS();
    }

    int flags = quicpro_how2flags(how, how_len);

    int rc = quiche_conn_stream_shutdown(s->conn, (uint64_t)stream_id, flags, 0);

    if (rc != 0) {
        quicpro_throw(rc);
        RETURN_FALSE;
    }

    RETURN_TRUE;
}
