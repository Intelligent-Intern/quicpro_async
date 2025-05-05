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
 * Strap in, because from here on out you’ll see a blend of Zend macros,
 * quiche calls, and careful resource management to bring the future
 * of web transport into your PHP scripts.
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

/* ──────────────────────────────────────────────────────────────────────────
 * 1) Function Prototypes
 *
 * PHP_FUNCTION declarations ensure the engine emits the correct
 * zif_quicpro_* symbols at compile time. This lets PHP scripts call
 * quicpro_connect(), quicpro_send_request(), and all other APIs.
 *───────────────────────────────────────────────────────────────────────────*/
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

/* ──────────────────────────────────────────────────────────────────────────
 * 2) Module Startup / Info Prototypes
 *
 * These functions hook into PHP’s lifecycle:
 *   • PHP_MINIT_FUNCTION registers resources, classes, constants.
 *   • PHP_MINFO_FUNCTION prints our extension info in phpinfo().
 *───────────────────────────────────────────────────────────────────────────*/
PHP_MINIT_FUNCTION(quicpro_async);
PHP_MINFO_FUNCTION(quicpro_async);

/* ---------------------------------------------------------------------------
 * Resource Registration
 *
 * We register a resource type "quicpro" with PHP, assigning it a unique
 * resource ID (le_quicpro). Each quicpro_connect() call returns one of
 * these resources, which encapsulates a quicpro_session_t pointer.
 * PHP will automatically call quicpro_session_dtor() when the resource
 * is garbage-collected or explicitly freed.
 * ------------------------------------------------------------------------*/
static int le_quicpro;   /* Unique resource ID for quicpro_session_t */

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
    /* Retrieve our session object from the resource pointer */
    quicpro_session_t *s = (quicpro_session_t *) res->ptr;
    if (!s) {
        /* Nothing to free; possible double-free guard */
        return;
    }

    /*
     * HTTP/3 context cleanup:
     * quiche_h3_conn_free() releases all memory held by the HTTP/3 layer.
     */
    if (s->h3) {
        quiche_h3_conn_free(s->h3);
    }

    /*
     * QUIC connection cleanup:
     * quiche_conn_free() tears down the QUIC state machine,
     * including TLS 1.3 session teardown.
     */
    if (s->conn) {
        quiche_conn_free(s->conn);
    }

    /*
     * HTTP/3 config cleanup:
     * quiche_h3_config_free() frees memory allocated for header encodings.
     */
    if (s->h3_cfg) {
        quiche_h3_config_free(s->h3_cfg);
    }

    /*
     * Core quiche_config cleanup:
     * Each session holds a pointer to a shared quiche_config. In
     * this build we assume one config per session, so free it here.
     * Cast away const qualifier because quiche_config_free() expects
     * a non-const pointer.
     */
    if (s->cfg) {
        quiche_config_free((quiche_config *)s->cfg);
    }

    /*
     * Underlying UDP socket:
     * When sock >= 0, we have an open fd that must be closed to
     * avoid file descriptor leaks.
     */
    if (s->sock >= 0) {
        close(s->sock);
    }

    /*
     * Finally, free the session structure itself. This also
     * implicitly releases memory for the host string and
     * ticket buffer embedded in the struct.
     */
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
    "quicpro_async",          /* Extension name in phpinfo() */
    quicpro_funcs,            /* Function table */
    PHP_MINIT(quicpro_async), /* MINIT handler */
    NULL,                     /* MSHUTDOWN handler */
    NULL,                     /* RINIT handler */
    NULL,                     /* RSHUTDOWN handler */
    PHP_MINFO(quicpro_async), /* MINFO handler */
    PHP_QUICPRO_VERSION,      /* Version (string) */
    STANDARD_MODULE_PROPERTIES
};
ZEND_GET_MODULE(quicpro_async)

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
    /* On Windows, startup Winsock (required for sockets API) */
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);
#endif

    /*
     * Register the quicpro resource:
     *    - quicpro_session_dtor will be called when the resource is freed.
     *    - No persistent des- tuctor is needed (NULL).
     *    - The name "quicpro" appears in PHP error messages if misused.
     */
    le_quicpro = zend_register_list_destructors_ex(
        quicpro_session_dtor,  /* Destructor callback */
        NULL,                  /* Persistent destructor (unused) */
        "quicpro",             /* Resource type name */
        module_number          /* Module identifier */
    );

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
