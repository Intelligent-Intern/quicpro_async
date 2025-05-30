/*
 * quicpro_async – Native QUIC / HTTP/3 Extension for PHP
 * ------------------------------------------------------
 *
 * Welcome to the heart of the quicpro_async PHP extension, where we
 * marry the performance of QUIC and HTTP/3 with the ease of PHP.
 *
 * QUIC is a modern, UDP‑based transport protocol that integrates TLS 1.3
 * directly into its handshake, reducing latency by eliminating one
 * round trip compared to TCP+TLS. HTTP/3 sits atop QUIC, bringing
 * reliable, ordered streams and multiplexing without head‑of‑line blocking.
 *
 * This extension lets PHP applications:
 *   • Act as HTTP/3 clients: open QUIC sessions, send requests, stream
 *     bodies, and receive responses.
 *   • Act as HTTP/3 servers: accept QUIC connections, upgrade to
 *     WebSockets, echo data, and serve fibers in a non‑blocking fashion.
 *   • Export and import TLS session tickets for zero‑RTT reconnects,
 *     preserving performance across forks or warm serverless containers.
 *   • Collect diagnostics: packet counters, RTT, congestion window size,
 *     and live QLOG traces for deep inspection.
 *
 * Why upgrade to QUIC?
 *   • Lower TTFB on lossy or high‑latency links (mobile networks, satellite).
 *   • Single UDP socket per origin instead of many TCP connections.
 *   • Built‑in encryption with modern ciphers and forward secrecy.
 *   • Multiplexing streams without head‑of‑line blocking improves
 *     concurrency and reduces CPU usage under load.
 *
 * Core Concepts:
 *   1. quicpro_connect(): Establishes a QUIC session to a remote host,
 *      performs the TLS 1.3 handshake, and returns a PHP resource.
 *   2. quicpro_send_request(): Encodes HTTP/3 headers and optional body
 *      into a new QUIC stream.
 *   3. quicpro_receive_response(): Reads incoming HTTP/3 frames,
 *      reconstructs headers and body, and returns them to PHP.
 *   4. quicpro_poll(): Advances the QUIC event loop, sending/receiving
 *      UDP packets, handling retries, timeouts, and busy‑poll yields.
 *   5. quicpro_close(): Gracefully closes the QUIC connection.
 *
 * On the server side:
 *   • quicpro_new_config(): Prepare a reusable quiche_config with TLS
 *     certificates, ALPN settings, timeouts, and more.
 *   • Quicpro\Server and Quicpro\Session classes: Accept incoming
 *     connections, upgrade to WebSockets, and read/write streams.
 *   • Shared‑memory ticket rings for multi‑worker zero‑RTT across forks.
 *
 * This file wires everything into PHP’s engine:
 *   • Registers PHP_FUNCTION entry points.
 *   • Handles MINIT to register resources and classes.
 *   • Implements MINFO so phpinfo() shows our support and quiche version.
 *
 * --------------------------------------------------------------------------
 * TLS Session Resumption & Multi-Worker Scaling (SESSION TICKET SYSTEM)
 * --------------------------------------------------------------------------
 *
 * What does this extension do for TLS session resumption, zero‑RTT, and scaling?
 *
 *  - QUIC and TLS 1.3 can use "session tickets" for instant, zero‑RTT reconnects.
 *  - These tickets are encrypted blobs given to the client at handshake end.
 *  - A client presents a ticket next time; the handshake skips a round-trip.
 *  - In PHP multi-worker servers, each worker must see all tickets to support resumption.
 *
 * This extension implements FIVE retention strategies for session tickets:
 *   1. Live object: keep the `$sess` resource in a global variable (single-process).
 *   2. Ticket cache: export ticket, store in APCu/Redis/file, import in a new process.
 *   3. Warm container: container/serverless (Lambda) – re-use across invocations.
 *   4. Shared-memory LRU: built-in shm cache, all forked workers access the same ring.
 *   5. Stateless LB: anycast, tickets follow the client (import/export).
 *
 * The default mode (AUTO) chooses the right one for the PHP context:
 *   - One PHP process → live object.
 *   - FPM/CLI with multiple children → ticket cache.
 *   - After fork() and if shm available → shared-memory LRU ring.
 *
 * Configuration is possible by both:
 *   - PHP.INI: for sysadmins (see below).
 *   - PHP code: pass as 'session_mode', 'shm_size', 'shm_path', etc. in Config/new().
 *
 * Example php.ini entries:
 *     quicpro.shm_size        = 131072             ; 128kB ~ 120 tickets in ring
 *     quicpro.shm_path        = /quicpro_ring      ; shm_open() path
 *     quicpro.session_mode    = 0                  ; AUTO, or 1=LIVE, 2=TICKET, 3=WARM, 4=SHM_LRU
 *
 * Example in PHP code:
 *     $cfg = Quicpro\Config::new([
 *         'session_mode' => QUICPRO_SESSION_SHM_LRU,
 *         'alpn'         => ['h3'],
 *     ]);
 *
 * All constants:
 *     QUICPRO_SESSION_AUTO        (0)
 *     QUICPRO_SESSION_LIVE        (1)
 *     QUICPRO_SESSION_TICKET      (2)
 *     QUICPRO_SESSION_WARM        (3)
 *     QUICPRO_SESSION_SHM_LRU     (4)
 *
 * See README.md and docs/TICKET_RESUMPTION_MULTIWORKER.md for
 * full demos of zero-RTT, worker scaling, and practical multi-process setups.
 *
 * You do NOT need to set php.ini values if you prefer config array parameters.
 * Both mechanisms are always available.
 */

#include <stddef.h>                    /* size_t, NULL */
#include "session.h"                   /* quicpro_session_t definition */
#include "php_quicpro.h"               /* PHP_FUNCTION prototypes, macros */
#include "php_quicpro_arginfo.h"       /* Generated arginfo for reflection */

#include <ext/standard/info.h>         /* phpinfo() helpers */
#include <quiche.h>                    /* quiche QUIC + HTTP/3 implementation */

#ifdef PHP_WIN32
# include <winsock2.h>                 /* Windows sockets init */
# include <ws2tcpip.h>                 /* getaddrinfo on Windows */
#else
# include <sys/socket.h>               /* socket(), struct sockaddr */
# include <netinet/in.h>               /* sockaddr_in, sockaddr_in6 */
# include <arpa/inet.h>                /* inet_pton, htons */
# include <unistd.h>                   /* close() */
#endif

#include <string.h>                    /* for strerror, strcpy, etc. */
#include <php.h>

/* ──────────────────────────────────────────────────────────────────────────
 * Global Error Buffer (thread-local for ZTS, otherwise global)
 *───────────────────────────────────────────────────────────────────────────*/
#ifndef QUICPRO_ERR_LEN
#  define QUICPRO_ERR_LEN 256
#endif

#if defined(ZTS) && (PHP_VERSION_ID < 80200)
# include <TSRM.h>
ZEND_TLS char quicpro_last_error[QUICPRO_ERR_LEN];
#else
char quicpro_last_error[QUICPRO_ERR_LEN];
#endif

void quicpro_set_error(const char *msg)
{
    if (!msg) {
        quicpro_last_error[0] = '\0';
        return;
    }
    strncpy(quicpro_last_error, msg, QUICPRO_ERR_LEN - 1);
    quicpro_last_error[QUICPRO_ERR_LEN - 1] = '\0';
}

const char *quicpro_get_error(void)
{
    return quicpro_last_error;
}



/* ---------------------------------------------------------------------------
 * Resource Registration
 *
 * We register a resource type "quicpro" with PHP, assigning it a unique
 * resource ID (le_quicpro). Each quicpro_connect() call returns one of
 * these resources, which encapsulates a quicpro_session_t pointer.
 * PHP will automatically call quicpro_session_dtor() when the resource
 * is garbage-collected or explicitly freed.
 * ------------------------------------------------------------------------*/
int le_quicpro;   /* Unique resource ID for quicpro_session_t */

int le_quicpro_session;

zend_class_entry *quicpro_ce_exception;
zend_class_entry *quicpro_ce_invalid_state;
zend_class_entry *quicpro_ce_unknown_stream;
zend_class_entry *quicpro_ce_stream_blocked;
zend_class_entry *quicpro_ce_stream_limit;
zend_class_entry *quicpro_ce_final_size;
zend_class_entry *quicpro_ce_stream_stopped;
zend_class_entry *quicpro_ce_fin_expected;
zend_class_entry *quicpro_ce_invalid_fin_state;
zend_class_entry *quicpro_ce_done;
zend_class_entry *quicpro_ce_congestion_control;
zend_class_entry *quicpro_ce_too_many_streams;

/**
 * quicpro_session_dtor()
 *
 * Destructor for the "quicpro" resource. Called automatically by PHP
 * when the last reference to a Session resource is released.
 * It must:
 *   1. Free the HTTP/3 context (quiche_h3_conn_free).
 *   2. Free the QUIC connection (quiche_conn_free).
 *   3. Free the HTTP/3 config (quiche_h3_config_free).
 *   4. Free the shared quiche_config if no longer used.
 *   5. Close the UDP socket.
 *   6. Release the allocated quicpro_session_t struct via efree().
 */
static void quicpro_session_dtor(zend_resource *res)
{
    quicpro_session_t *s = (quicpro_session_t *) res->ptr;
    if (!s) {
        return;
    }
    if (s->h3) {
        quiche_h3_conn_free(s->h3);
    }
    if (s->conn) {
        quiche_conn_free(s->conn);
    }
    if (s->h3_cfg) {
        quiche_h3_config_free(s->h3_cfg);
    }
    if (s->cfg) {
        quiche_config_free((quiche_config *)s->cfg);
    }
    if (s->sock >= 0) {
        close(s->sock);
    }
    efree(s);
}

/* ---------------------------------------------------------------------------
 * Function Table
 *
 * Map PHP function names to their C implementations and
 * provide arginfo for engine validation and reflection.
 * ------------------------------------------------------------------------*/
static const zend_function_entry quicpro_funcs[] = {
    PHP_FE(quicpro_connect,               arginfo_quicpro_connect)
    PHP_FE(quicpro_close,                 arginfo_quicpro_close)
    PHP_FE(quicpro_send_request,          arginfo_quicpro_send_request)
    PHP_FE(quicpro_receive_response,      arginfo_quicpro_receive_response)
    PHP_FE(quicpro_poll,                  arginfo_quicpro_poll)
    PHP_FE(quicpro_cancel_stream,         arginfo_quicpro_cancel_stream)
    PHP_FE(quicpro_export_session_ticket, arginfo_quicpro_export_session_ticket)
    PHP_FE(quicpro_import_session_ticket, arginfo_quicpro_import_session_ticket)
    PHP_FE(quicpro_set_ca_file,           arginfo_quicpro_set_ca_file)
    PHP_FE(quicpro_set_client_cert,       arginfo_quicpro_set_client_cert)
    PHP_FE(quicpro_get_last_error,        arginfo_quicpro_get_last_error)
    PHP_FE(quicpro_get_stats,             arginfo_quicpro_get_stats)
    PHP_FE(quicpro_version,               arginfo_quicpro_version)
    PHP_FE_END
};

PHP_FUNCTION(quicpro_version)
{
    RETURN_STRING(PHP_QUICPRO_VERSION);
}


/* ---------------------------------------------------------------------------
 * quicpro_get_last_error
 *
 * Returns the last error message set by quicpro_set_error().
 * ------------------------------------------------------------------------*/
PHP_FUNCTION(quicpro_get_last_error)
{
    RETURN_STRING(quicpro_get_error());
}


/* ---------------------------------------------------------------------------
 * PHP_MINIT_FUNCTION(quicpro_async)
 *
 * Module initialization: register the "quicpro" resource type and its destructor.
 * On Windows, also initialize the Winsock library.
 * Returns SUCCESS on success or FAILURE on error.
 * ------------------------------------------------------------------------*/
PHP_MINIT_FUNCTION(quicpro_async)
{
#ifdef PHP_WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);
#endif

    le_quicpro = zend_register_list_destructors_ex(
        quicpro_session_dtor,
        NULL,
        "quicpro",
        module_number
    );

    quicpro_set_error(NULL);

    return SUCCESS;
}

/* ---------------------------------------------------------------------------
 * PHP_MINFO_FUNCTION(quicpro_async)
 *
 * Prints extension status and quiche version in phpinfo() output.
 * This helps administrators verify that the extension is loaded and
 * which underlying quiche library version is in use.
 * ------------------------------------------------------------------------*/
PHP_MINFO_FUNCTION(quicpro_async)
{
    php_info_print_table_start();
    php_info_print_table_row(2,
        "quicpro_async support", "enabled"
    );
    php_info_print_table_row(2,
        "libquiche version", quiche_version()
    );
    php_info_print_table_end();
}

/* ---------------------------------------------------------------------------
 * Module Entry / Boilerplate
 *
 * This struct tells the PHP engine about our extension:
 *   • Name ("quicpro_async")
 *   • Version (PHP_QUICPRO_VERSION from php_quicpro.h)
 *   • Function table (quicpro_funcs)
 *   • Life-cycle hooks (MINIT and MINFO)
 *   • Module properties (persistent, globals)
 * ZEND_GET_MODULE exposes the entry point for dynamic loading.
 * ------------------------------------------------------------------------*/
zend_module_entry quicpro_async_module_entry = {
    STANDARD_MODULE_HEADER,
    "quicpro_async",
    quicpro_funcs,
    PHP_MINIT(quicpro_async),
    NULL,
    NULL,
    NULL,
    PHP_MINFO(quicpro_async),
    PHP_QUICPRO_VERSION,
    STANDARD_MODULE_PROPERTIES
};


#ifdef COMPILE_DL_QUICPRO_ASYNC
ZEND_GET_MODULE(quicpro_async)
#endif
