/*  include/php_quicpro.h  ── public interface for the php-quicpro_async extension
 *
 *  Dieses Header wird von allen C-Quellen der Extension verwendet und
 *  definiert Versions-, Konstanten-, Typ- und Exportsymbole.
 */

#ifndef PHP_QUICPRO_H
#define PHP_QUICPRO_H

/* ── Versions- und Größen-Konstanten ─────────────────────────────────────── */
#ifndef PHP_QUICPRO_VERSION
#  define PHP_QUICPRO_VERSION      "0.1.0"
#endif

#ifndef QUICPRO_MAX_TICKET_SIZE
#  define QUICPRO_MAX_TICKET_SIZE  512    /* muss mit session.h übereinstimmen */
#endif

/* ── Standard- und PHP-Header ───────────────────────────────────────────── */
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <php.h>
#include <zend_object_handlers.h>
#include <stdint.h>
#include <string.h>
#include <stdatomic.h>

/* Shim für PHP-8.4-Kompatibilität */
#if PHP_VERSION_ID >= 80400 && PHP_VERSION_ID < 80500
#  define QUICPRO_PHP84_COMPAT 1
#endif

/* ── Extension-spezifische Includes ─────────────────────────────────────── */
#include "session.h"   /* quicpro_session_t, QuicSession-Klasse */
#include "cancel.h"    /* Exception-Mapping */
#include "tls.h"       /* TLS-Optionen und Ticket APIs, falls vorhanden */

/* ── Exception-Klassen (in MINIT registriert) ───────────────────────────── */
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

/* ── Resource-IDs (in MINIT registriert) ─────────────────────────────────── */
extern int le_quicpro_session;  /* quicpro_session_t* */
extern int le_quicpro_cfg;      /* quiche_config*     */
extern int le_quicpro_perf;     /* perf mmap page*    */

/* ── Zend-Objekt-Wrapper für \Quicpro\Session ───────────────────────────── */
typedef struct {
    quicpro_session_t *sess;  /* nativer Session-Pointer */
    zend_object        std;   /* PHP-Objekt-Header      */
} quicpro_session_object;

/* zend_object* → quicpro_session_object* */
static inline quicpro_session_object *
php_quicpro_obj_from_zend(zend_object *obj)
{
    return (quicpro_session_object *)
        ((char*)obj - XtOffsetOf(quicpro_session_object, std));
}

/* zval* → quicpro_session_t* oder NULL */
static inline quicpro_session_t *quicpro_obj_fetch(zval *zobj)
{
    return Z_TYPE_P(zobj) == IS_OBJECT
         ? php_quicpro_obj_from_zend(Z_OBJ_P(zobj))->sess
         : NULL;
}

/* ── Thread-lokaler Fehler-String ───────────────────────────────────────── */
#ifndef QUICPRO_ERR_LEN
#  define QUICPRO_ERR_LEN 256
#endif

extern __thread char quicpro_last_error[QUICPRO_ERR_LEN];

static inline void quicpro_set_error(const char *msg)
{
    if (!msg) {
        quicpro_last_error[0] = '\0';
        return;
    }
    strncpy(quicpro_last_error, msg, QUICPRO_ERR_LEN - 1);
    quicpro_last_error[QUICPRO_ERR_LEN - 1] = '\0';
}

/* ── perf_event mmap (Linux) ─────────────────────────────────────────────── */
#ifdef __linux__
#  include <linux/perf_event.h>
   typedef struct perf_event_mmap_page quicpro_perf_page_t;
#else
   typedef struct { char _unused; } quicpro_perf_page_t;
#endif

#endif /* PHP_QUICPRO_H */
