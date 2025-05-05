/* include/php_quicpro.h  ── Public interface for the php-quicpro_async extension
 *
 * This header is included by every C source file in the extension.
 * It centralizes the core type definitions, version constants, exception
 * and resource identifiers, and utility functions used throughout the codebase.
 * By keeping all of these declarations in one place, we ensure a consistent
 * view of the extension’s API and avoid duplication across modules.
 */

#ifndef PHP_QUICPRO_H
#define PHP_QUICPRO_H

#ifdef HAVE_CONFIG_H
#  include "config.h"      /* Autoconf-generated configuration (paths, features) */
#endif

/*─────────────────────────────────────────────────────────────────────────────
 * Standard PHP and C library includes
 *
 * We bring in the PHP API and Zend object handler definitions to interact
 * with the engine.  We also include fixed-width integer types, string
 * utilities, and atomic operations for thread-local ticket management.
 *
 *   <php.h>                 – Core PHP embedding API (functions, macros)
 *   <zend_object_handlers.h>– Definitions for custom object handlers
 *   <stdint.h>              – uint8_t, uint32_t, etc.
 *   <string.h>              – memcpy, strncpy, strlen
 *   <stdatomic.h>           – C11 atomics for session-ticket ring buffer
 *─────────────────────────────────────────────────────────────────────────────*/
#include <php.h>
#include <zend_object_handlers.h>
#include <stdint.h>
#include <string.h>
#include <stdatomic.h>

/*─────────────────────────────────────────────────────────────────────────────
 * Extension version and constants
 *
 * The version string must be updated with every release of the extension.
 * The maximum ticket size must match the buffer defined in session.h
 * to ensure that session tickets can be safely exported and imported
 * without risk of overflow or truncation.
 *─────────────────────────────────────────────────────────────────────────────*/
#ifndef PHP_QUICPRO_VERSION
#  define PHP_QUICPRO_VERSION      "0.1.0"
#endif

#ifndef QUICPRO_MAX_TICKET_SIZE
#  define QUICPRO_MAX_TICKET_SIZE  512
#endif

/*─────────────────────────────────────────────────────────────────────────────
 * Forward declarations for core extension modules
 *
 * We pull in the headers for our session management, error-to-exception
 * mapping, and TLS/session-ticket configuration logic.  This allows
 * any C file including php_quicpro.h to access the full set of APIs
 * and type definitions without worrying about include order.
 *─────────────────────────────────────────────────────────────────────────────*/
#include "session.h"   /* quicpro_session_t, QuicSession class definition */
#include "cancel.h"    /* quicpro_throw() – maps quiche errors to PHP exceptions */
#include "tls.h"       /* quicpro_set_ca_file(), quicpro_import/export_session_ticket() */

/*─────────────────────────────────────────────────────────────────────────────
 * Exception class entries (registered in PHP_MINIT)
 *
 * We declare extern pointers to the zend_class_entry for each type of
 * Quicpro\Exception.  These are initialized at module startup and used
 * by quicpro_throw() to instantiate the correct subclass based on
 * libquiche error codes.  By centralizing these entries here, we
 * decouple error mapping from the rest of the code.
 *─────────────────────────────────────────────────────────────────────────────*/
extern zend_class_entry
    *quicpro_ce_exception,           /* Base exception class */
    *quicpro_ce_invalid_state,       /* QUIC stream in wrong state */
    *quicpro_ce_unknown_stream,      /* Reference to non‑existent stream */
    *quicpro_ce_stream_blocked,      /* Stream blocked by flow control */
    *quicpro_ce_stream_limit,        /* Too many open streams */
    *quicpro_ce_final_size,          /* Final size violation */
    *quicpro_ce_stream_stopped,      /* Stream stopped by peer */
    *quicpro_ce_fin_expected,        /* FIN not expected */
    *quicpro_ce_invalid_fin_state,   /* FIN in invalid state */
    *quicpro_ce_done,                /* Operation already done */
    *quicpro_ce_congestion_control,  /* Congestion control error */
    *quicpro_ce_too_many_streams;    /* Too many streams opened */

/*─────────────────────────────────────────────────────────────────────────────
 * Resource type identifiers (registered in PHP_MINIT)
 *
 * These integer handles allow PHP to track two kinds of resources:
 *   - quicpro_session: a live QUIC + HTTP/3 connection
 *   - quicpro_cfg:     a reusable quiche_config pointer
 *   - quicpro_perf:    a perf_event mmap page for instrumentation
 *
 * Zend uses these IDs to ensure that resources are freed with the correct
 * destructors when they fall out of scope.
 *─────────────────────────────────────────────────────────────────────────────*/
extern int le_quicpro_session;  /* quicpro_session_t* */
extern int le_quicpro_cfg;      /* quiche_config*     */
extern int le_quicpro_perf;     /* perf mmap page*    */

/*─────────────────────────────────────────────────────────────────────────────
 * Zend object wrapper for Quicpro\Session
 *
 * PHP objects of class \Quicpro\Session hold a pointer to our native
 * quicpro_session_t struct in the `sess` field.  The `std` field
 * embeds Zend’s own object header, which tracks properties, handlers,
 * and the garbage collector’s reference count.  This two‑field struct
 * lets us tie native resources to PHP objects seamlessly.
 *─────────────────────────────────────────────────────────────────────────────*/
typedef struct _quicpro_session_object {
    quicpro_session_t *sess;  /* Native pointer to the session data */
    zend_object        std;   /* Zend-managed object header */
} quicpro_session_object;

/*─────────────────────────────────────────────────────────────────────────────
 * php_quicpro_obj_from_zend()
 *
 * Convert a raw zend_object pointer into our wrapper struct pointer.
 * We compute the base address of quicpro_session_object by subtracting
 * the offset of `std` within the struct.  This inline helper is used
 * in every method and destructor that needs to recover the native
 * session pointer from the PHP object.
 *─────────────────────────────────────────────────────────────────────────────*/
static inline quicpro_session_object *
php_quicpro_obj_from_zend(zend_object *obj)
{
    return (quicpro_session_object *)
        ((char*)obj - XtOffsetOf(quicpro_session_object, std));
}

/*─────────────────────────────────────────────────────────────────────────────
 * quicpro_obj_fetch()
 *
 * Given a zval representing a Quicpro\Session instance, return
 * the underlying quicpro_session_t* or NULL on type mismatch.
 * This helper checks that the zval is indeed an object, then
 * uses php_quicpro_obj_from_zend() to recover the wrapper struct.
 * User‑facing methods call this to obtain the native session handle.
 *─────────────────────────────────────────────────────────────────────────────*/
static inline quicpro_session_t *quicpro_obj_fetch(zval *zobj)
{
    if (Z_TYPE_P(zobj) != IS_OBJECT) {
        return NULL;
    }
    quicpro_session_object *intern = php_quicpro_obj_from_zend(Z_OBJ_P(zobj));
    return intern->sess;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Thread-local storage for last error string
 *
 * We maintain a per‑thread buffer quicpro_last_error[] of length
 * QUICPRO_ERR_LEN to hold the most recent error message.  Since typical
 * PHP deployments are single‑threaded per process (e.g. FPM), this
 * buffer effectively serves as a process‑wide error store in practice.
 *─────────────────────────────────────────────────────────────────────────────*/
#ifndef QUICPRO_ERR_LEN
#  define QUICPRO_ERR_LEN 256
#endif
extern __thread char quicpro_last_error[QUICPRO_ERR_LEN];

/*─────────────────────────────────────────────────────────────────────────────
 * quicpro_set_error()
 *
 * Store or clear the last error message.  Passing a NULL pointer clears
 * the buffer.  Otherwise, the provided string is copied (up to the buffer
 * length minus one) and NUL‑terminated.  This function is used by our
 * internal APIs to record human‑readable error context retrievable
 * via quicpro_get_last_error() in PHP.
 *─────────────────────────────────────────────────────────────────────────────*/
static inline void quicpro_set_error(const char *msg)
{
    if (!msg) {
        quicpro_last_error[0] = '\0';
        return;
    }
    strncpy(quicpro_last_error, msg, QUICPRO_ERR_LEN - 1);
    quicpro_last_error[QUICPRO_ERR_LEN - 1] = '\0';
}

/*─────────────────────────────────────────────────────────────────────────────
 * quicpro_fetch_config()
 *
 * Retrieve a quiche_config* from a PHP resource zval.  This function
 * is implemented in config.c and ensures that the resource is valid
 * and not yet frozen by a connection.  On success, it returns the
 * native pointer for use in quiche_connect() and related calls.
 *─────────────────────────────────────────────────────────────────────────────*/
extern quiche_config *quicpro_fetch_config(zval *zcfg);

/*─────────────────────────────────────────────────────────────────────────────
 * quicpro_ticket_ring_put()
 *
 * Enqueue a TLS session ticket into the process-wide ring buffer.
 * This allows tickets exported from one session to be imported
 * into another, enabling 0‑RTT resumption across client reconnects.
 * The ring buffer implementation uses atomic operations for safety.
 *─────────────────────────────────────────────────────────────────────────────*/
extern void quicpro_ticket_ring_put(const uint8_t *ticket, size_t len);

#endif /* PHP_QUICPRO_H */
