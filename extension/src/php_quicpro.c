/*
 * Welcome to the QUICPro PHP Extension
 * -------------------------------------
 * Imagine your PHP application sending and receiving data as if it were
 * streaming a live video: fast, reliable, and secure. This extension
 * makes that possible by using the QUIC protocol and HTTP/3 under the hood.
 * CEOs, think of it as the invisible engine that turbocharges your web calls.
 */
#include <stddef.h>
#include "session.h"             /* Core session data structure and cleanup routines   */
#include "php_quicpro.h"         /* Declarations of every PHP_FUNCTION we implement    */
#include "php_quicpro_arginfo.h"

#include <ext/standard/info.h>   /* Für phpinfo()-Ausgabe                             */
#include <quiche.h>              /* Unterliegender QUIC + HTTP/3-Engine                */

#ifdef PHP_WIN32
# include <winsock2.h>
# include <ws2tcpip.h>
#else
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <unistd.h>            /* close()                                            */
#endif

/* ──────────────────────────────────────────────────────────────────────────
 *  1) Funktions-Prototypen
 *     Hier deklarieren wir vorab alle PHP_FUNCTIONs, damit
 *     die zugehörigen zif_quicpro_* Symbole beim Kompilieren sichtbar sind.
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
 *  2) Module-Startup/-Info-Prototypen
 *     Damit die Referenzen auf zm_startup_quicpro_async und zm_info_quicpro_async
 *     in quicpro_async_module_entry korrekt aufgelöst werden.
 *───────────────────────────────────────────────────────────────────────────*/
PHP_MINIT_FUNCTION(quicpro_async);
PHP_MINFO_FUNCTION(quicpro_async);

/* ---------------------------------------------------------------------------
 *  Resource registration
 * ------------------------------------------------------------------------ */
static int le_quicpro;   /* unique ID PHP uses to track “quicpro” resources */

/**
 * quicpro_session_dtor()
 *
 * PHP ruft das automatisch auf, wenn die letzte Referenz auf eine Session
 * verschwindet. Wir geben HTTP/3 → QUIC → Socke frei und schließlich
 * den malloc()/ecalloc()-Speicher.
 */
static void quicpro_session_dtor(zend_resource *res)
{
    quicpro_session_t *s = (quicpro_session_t *) res->ptr;
    if (!s) return;

    if (s->h3)      quiche_h3_conn_free(s->h3);
    if (s->conn)    quiche_conn_free(s->conn);
    if (s->h3_cfg)  quiche_h3_config_free(s->h3_cfg);
    /* Cast hier, um die Warnung über const-Qualifier zu beseitigen */
    if (s->cfg)     quiche_config_free((quiche_config *)s->cfg);
    if (s->sock > 0) close(s->sock);

    efree(s);
}

/* ---------------------------------------------------------------------------
 *  Function table
 * ------------------------------------------------------------------------ */
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
 *  Module entry / boiler-plate
 * ------------------------------------------------------------------------ */
zend_module_entry quicpro_async_module_entry = {
    STANDARD_MODULE_HEADER,
    "quicpro_async",          /* Extension-Name in phpinfo()                   */
    quicpro_funcs,            /* Functionstabelle                              */
    PHP_MINIT(quicpro_async), /* MINIT                                         */
    NULL,                     /* MSHUTDOWN                                     */
    NULL,                     /* RINIT                                         */
    NULL,                     /* RSHUTDOWN                                     */
    PHP_MINFO(quicpro_async), /* MINFO                                         */
    PHP_QUICPRO_VERSION,      /* Version (aus php_quicpro.h)                   */
    STANDARD_MODULE_PROPERTIES
};
ZEND_GET_MODULE(quicpro_async)

/* ---------------------------------------------------------------------------
 *  MINIT: one-time initialisation
 * ------------------------------------------------------------------------ */
PHP_MINIT_FUNCTION(quicpro_async)
{
#ifdef PHP_WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);
#endif
    le_quicpro = zend_register_list_destructors_ex(
        quicpro_session_dtor,  /* Destructor                                   */
        NULL,                  /* Persistent-Destructor                        */
        "quicpro",             /* Resource-Type-Name                           */
        module_number
    );
    return SUCCESS;
}

/* ---------------------------------------------------------------------------
 *  MINFO: Ausgabe für phpinfo()
 * ------------------------------------------------------------------------ */
PHP_MINFO_FUNCTION(quicpro_async)
{
    php_info_print_table_start();
    php_info_print_table_row(2, "quicpro_async support", "enabled");
    php_info_print_table_row(2, "libquiche version",     quiche_version());
    php_info_print_table_end();
}
