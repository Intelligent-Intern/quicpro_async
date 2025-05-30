/* include/php_quicpro.h  ── Public interface for the php-quicpro_async extension
 *
 * This header file provides the foundational interface, data types, constants,
 * exception and resource declarations, and function prototypes for the
 * php-quicpro_async PHP extension.
 *
 * Designed for inclusion in every C source file of the extension, this header
 * ensures consistency and avoids duplication. All public APIs, core structures,
 * and integration points are declared here to support maintainable, extensible,
 * and reviewable code.
 */

#ifndef PHP_QUICPRO_H
#define PHP_QUICPRO_H

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <php.h>
#include <zend_object_handlers.h>
#include <stdint.h>
#include <string.h>
#include <stdatomic.h>

/* -----------------------------------------------------------------------------
 * Extension Version and Global Constants
 */
#ifndef PHP_QUICPRO_VERSION
#  define PHP_QUICPRO_VERSION      "0.1.0"
#endif

#ifndef QUICPRO_MAX_TICKET_SIZE
#  define QUICPRO_MAX_TICKET_SIZE  512
#endif

#include "session.h"    /* Native session structure and PHP object class */
#include "cancel.h"     /* Exception mapping and error handling */
#include "tls.h"        /* TLS configuration and session ticket management */
#include "config.h"     /* Config resource definitions */
#include "connect.h"    /* QUIC connection logic */
#include "http3.h"      /* HTTP/3 support and helpers */
#include "mcp.h"        /* Model Context Protocol interface */
#include "poll.h"       /* Event loop/poll support */
#include "websocket.h"  /* WebSocket/CONNECT helpers and API */

/* -----------------------------------------------------------------------------
 * Exception Class Entry Declarations
 */
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
    *quicpro_ce_too_many_streams,
    *quicpro_ce_mcp_exception,
    *quicpro_ce_mcp_connection_error,
    *quicpro_ce_mcp_protocol_error,
    *quicpro_ce_mcp_timeout,
    *quicpro_ce_mcp_data_error,
    *quicpro_ce_ws_exception,
    *quicpro_ce_ws_connection_error,
    *quicpro_ce_ws_protocol_error,
    *quicpro_ce_ws_timeout,
    *quicpro_ce_ws_closed;

/* -----------------------------------------------------------------------------
 * Resource Type Identifiers
 */
extern int le_quicpro_session;
extern int le_quicpro_cfg;
extern int le_quicpro_perf;
extern int le_quicpro_mcp;
extern int le_quicpro_ws;

/* -----------------------------------------------------------------------------
 * Zend Object Wrappers for PHP Classes
 */
typedef struct _quicpro_session_object {
    quicpro_session_t *sess;
    zend_object        std;
} quicpro_session_object;

typedef struct _quicpro_mcp_object {
    void         *mcp_ctx;
    zend_object   std;
} quicpro_mcp_object;

typedef struct _quicpro_ws_object {
    void         *ws;
    zend_object   std;
} quicpro_ws_object;

/* -----------------------------------------------------------------------------
 * Inline Helpers for Accessing Native Resources
 */
static inline quicpro_session_object *
php_quicpro_obj_from_zend(zend_object *obj)
{
    return (quicpro_session_object *)
        ((char*)obj - XtOffsetOf(quicpro_session_object, std));
}

static inline quicpro_session_t *quicpro_obj_fetch(zval *zobj)
{
    if (Z_TYPE_P(zobj) != IS_OBJECT) return NULL;
    quicpro_session_object *intern = php_quicpro_obj_from_zend(Z_OBJ_P(zobj));
    return intern->sess;
}

static inline quicpro_mcp_object *
php_quicpro_mcp_obj_from_zend(zend_object *obj)
{
    return (quicpro_mcp_object *)
        ((char*)obj - XtOffsetOf(quicpro_mcp_object, std));
}

static inline quicpro_ws_object *
php_quicpro_ws_obj_from_zend(zend_object *obj)
{
    return (quicpro_ws_object *)
        ((char*)obj - XtOffsetOf(quicpro_ws_object, std));
}

/* -----------------------------------------------------------------------------
 * Thread-local Error Buffer
 */
#ifndef QUICPRO_ERR_LEN
#  define QUICPRO_ERR_LEN 256
#endif

#if defined(ZTS) && (PHP_VERSION_ID < 80200)
#  include <TSRM.h>
   extern ZEND_TLS char quicpro_last_error[QUICPRO_ERR_LEN];
#else
   extern char quicpro_last_error[QUICPRO_ERR_LEN];
#endif

void quicpro_set_error(const char *msg);
const char *quicpro_get_error(void);

extern quiche_config *quicpro_fetch_config(zval *zcfg);
extern void quicpro_ticket_ring_put(const uint8_t *ticket, size_t len);

#endif /* PHP_QUICPRO_H */

/* -----------------------------------------------------------------------------
 * PHP_FUNCTION Prototypes: Core QUIC/HTTP3 API
 */
PHP_FUNCTION(quicpro_connect);
PHP_FUNCTION(quicpro_close);
PHP_FUNCTION(quicpro_send_request);
PHP_FUNCTION(quicpro_receive_response);
PHP_FUNCTION(quicpro_poll);
PHP_FUNCTION(quicpro_cancel_stream);
PHP_FUNCTION(quicpro_export_session_ticket);
PHP_FUNCTION(quicpro_import_session_ticket);
PHP_FUNCTION(quicpro_set_ca_file);
PHP_FUNCTION(quicpro_set_client_cert);
PHP_FUNCTION(quicpro_get_last_error);
PHP_FUNCTION(quicpro_get_stats);
PHP_FUNCTION(quicpro_version);

/* -----------------------------------------------------------------------------
 * PHP_FUNCTION Prototypes: Model Context Protocol (Quicpro\MCP)
 */
PHP_FUNCTION(quicpro_mcp_connect);
PHP_FUNCTION(quicpro_mcp_disconnect);
PHP_FUNCTION(quicpro_mcp_list_tools);
PHP_FUNCTION(quicpro_mcp_invoke_tool);
PHP_FUNCTION(quicpro_mcp_list_resources);
PHP_FUNCTION(quicpro_mcp_get_resource);
PHP_FUNCTION(quicpro_mcp_fetch_data);
PHP_FUNCTION(quicpro_mcp_get_last_error);

