/*  php_quicpro.h  ── public interface for the php-quicpro extension
 *
 *  This header is included by every C file in the extension and exposes
 *  the shared types, resource IDs, and error‐handling helpers.
 */

#ifndef PHP_QUICPRO_H
#define PHP_QUICPRO_H

/* 1. Standard headers & feature toggles */
#ifdef HAVE_CONFIG_H
#   include "config.h"
#endif

#include <php.h>
#include <zend_object_handlers.h>
#include <stdint.h>
#include <string.h>
#include <stdatomic.h>

/* Shim PHP-8.4 helper APIs when compiling against 8.4 headers */
#if PHP_VERSION_ID >= 80400 && PHP_VERSION_ID < 80500
#   define QUICPRO_PHP84_COMPAT 1
#endif

/* Pull in the session struct, ticket ring, etc. */
#include "include/session.h"

/* Exception‐throwing helper; maps libquiche errors → PHP exceptions */
#include "cancel.h"

/* 2. Cross‐module symbols */

/* Exception classes (registered in MINIT) */
extern zend_class_entry
    *quicpro_ce_exception,
    *quicpro_ce_invalid_state,
    *quicpro_ce_unknown_stream,
    *quicpro_ce_stream_blocked,
    *quicpro_ce_stream_limit,
    *quicpro_ce_final_size,
    *quicpro_ce_stream_stopped,
    *quicpro_ce_fin_expected,
    *quicpro_ce_invalid_fin_state,
    *quicpro_ce_done,
    *quicpro_ce_congestion_control,
    *quicpro_ce_too_many_streams;

/* Resource identifiers (registered in MINIT) */
extern int le_quicpro_session;  /* quicpro_session_t* */
extern int le_quicpro_cfg;      /* quiche_config*     */
extern int le_quicpro_perf;     /* perf mmap page*    */

/* 3. Zend object wrapper for \Quicpro\Session */
typedef struct {
    quicpro_session_t *sess;  /* native session pointer */
    zend_object        std;   /* PHP object header       */
} quicpro_session_object;

/* Convert zend_object* → wrapper */
static inline quicpro_session_object *
php_quicpro_obj_from_zend(zend_object *obj)
{
    return (quicpro_session_object *)
        ((char*)obj - XtOffsetOf(quicpro_session_object, std));
}

/* Extract native session pointer or NULL */
static inline quicpro_session_t *quicpro_obj_fetch(zval *zobj)
{
    return Z_TYPE_P(zobj) == IS_OBJECT
         ? php_quicpro_obj_from_zend(Z_OBJ_P(zobj))->sess
         : NULL;
}

/* 4. Thread-local error buffer */
#ifndef QUICPRO_ERR_LEN
#   define QUICPRO_ERR_LEN 256
#endif

extern __thread char quicpro_last_error[QUICPRO_ERR_LEN];

/* Write or clear the last error message */
static inline void quicpro_set_error(const char *msg)
{
    if (!msg) {
        quicpro_last_error[0] = '\0';
        return;
    }
    strncpy(quicpro_last_error, msg, QUICPRO_ERR_LEN - 1);
    quicpro_last_error[QUICPRO_ERR_LEN - 1] = '\0';
}

/* 5. perf_event mmap alias */
#ifdef __linux__
#   include <linux/perf_event.h>
    typedef struct perf_event_mmap_page quicpro_perf_page_t;
#else
    typedef struct { char _unused; } quicpro_perf_page_t;
#endif

#endif /* PHP_QUICPRO_H */
