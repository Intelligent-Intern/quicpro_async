/*
 * Welcome to the QUICPro PHP Extension
 * -------------------------------------
 * Imagine your PHP application sending and receiving data as if it were
 * streaming a live video: fast, reliable, and secure. This extension
 * makes that possible by using the QUIC protocol and HTTP/3 under the hood.
 * CEOs, think of it as the invisible engine that turbocharges your web calls.
 */

#include "session.h"      /* Core session data structure and cleanup routines   */
#include "php_quicpro.h"  /* Declarations of every PHP_FUNCTION we implement    */
#include <ext/standard/info.h>  /* Lets us add rows to phpinfo() output         */
#include <quiche.h>       /* Underlying QUIC + HTTP/3 engine                    */

#ifdef PHP_WIN32
/* Windows needs Winsock initialisation before any socket APIs work. */
# include <winsock2.h>
# include <ws2tcpip.h>
#else
/* POSIX sockets -- the lingua franca of Unix-like systems. */
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <unistd.h>      /* close() */
#endif

/* ---------------------------------------------------------------------------
 *  Resource registration
 * ------------------------------------------------------------------------ */
static int le_quicpro;   /* unique ID PHP uses to track “quicpro” resources */

/**
 * quicpro_session_dtor()
 *
 * PHP calls this automatically when the last reference to a session is gone.
 * We walk through every layer (HTTP/3 → QUIC → socket) and free memory so the
 * child process remains leak-free even under heavy load.
 */
static void quicpro_session_dtor(zend_resource *res)
{
    quicpro_session_t *s = (quicpro_session_t *) res->ptr;
    if (!s) return;

    if (s->h3)      quiche_h3_conn_free(s->h3);
    if (s->conn)    quiche_conn_free(s->conn);
    if (s->h3_cfg)  quiche_h3_config_free(s->h3_cfg);
    if (s->cfg)     quiche_config_free(s->cfg);
    if (s->sock > 0) close(s->sock);

    efree(s);
}

/* ---------------------------------------------------------------------------
 *  Function table
 * ------------------------------------------------------------------------ */
/*
 * We omit arg-info structs for now (second argument = NULL).  That removes the
 * missing-header problem; you can add proper stubs later if you need reflection.
 */
static const zend_function_entry quicpro_funcs[] = {
    PHP_FE(quicpro_connect,               NULL)
    PHP_FE(quicpro_close,                 NULL)
    PHP_FE(quicpro_send_request,          NULL)
    PHP_FE(quicpro_receive_response,      NULL)
    PHP_FE(quicpro_poll,                  NULL)
    PHP_FE(quicpro_cancel_stream,         NULL)
    PHP_FE(quicpro_export_session_ticket, NULL)
    PHP_FE(quicpro_import_session_ticket, NULL)
    PHP_FE(quicpro_set_ca_file,           NULL)
    PHP_FE(quicpro_set_client_cert,       NULL)
    PHP_FE(quicpro_get_last_error,        NULL)
    PHP_FE(quicpro_get_stats,             NULL)
    PHP_FE(quicpro_version,               NULL)
    PHP_FE_END
};

/* ---------------------------------------------------------------------------
 *  Module entry / boiler-plate
 * ------------------------------------------------------------------------ */
zend_module_entry quicpro_async_module_entry = {
    STANDARD_MODULE_HEADER,
    "quicpro_async",          /* extension name as shown in phpinfo()          */
    quicpro_funcs,            /* function table                                */
    PHP_MINIT(quicpro_async), /* called once per PHP process start-up          */
    NULL,                     /* MSHUTDOWN – not needed                        */
    NULL,                     /* RINIT – per-request start-up (N/A)            */
    NULL,                     /* RSHUTDOWN – per-request shutdown (N/A)        */
    PHP_MINFO(quicpro_async), /* dumps a small phpinfo() table                 */
    PHP_QUICPRO_VERSION,      /* macro from php_quicpro.h                      */
    STANDARD_MODULE_PROPERTIES
};

ZEND_GET_MODULE(quicpro_async)

/* ---------------------------------------------------------------------------
 *  MINIT: one-time initialisation
 * ------------------------------------------------------------------------ */
PHP_MINIT_FUNCTION(quicpro_async)
{
#ifdef PHP_WIN32
    /* Winsock must be initialised exactly once in every Windows process. */
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);
#endif
    le_quicpro = zend_register_list_destructors_ex(
        quicpro_session_dtor,  /* destructor                                        */
        NULL,                  /* persistent-destructor (N/A for CLI/FPM)           */
        "quicpro",             /* resource type name visible to PHP (for gettype)   */
        module_number
    );
    return SUCCESS;
}

/* ---------------------------------------------------------------------------
 *  MINFO: phpinfo() output
 * ------------------------------------------------------------------------ */
PHP_MINFO_FUNCTION(quicpro_async)
{
    php_info_print_table_start();
    php_info_print_table_row(2, "quicpro_async support", "enabled");
    php_info_print_table_row(2, "libquiche version",     quiche_version());
    php_info_print_table_end();
}
