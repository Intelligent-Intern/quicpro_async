#include "php_quicpro.h"
#include "php_quicpro_arginfo.h"
#include <ext/standard/info.h>

#ifdef PHP_WIN32
# include <winsock2.h>
# include <ws2tcpip.h>
#else
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
#endif

int le_quicpro;

static void quicpro_session_dtor(zend_resource *res)
{
    quicpro_session_t *s = (quicpro_session_t *)res->ptr;
    if (!s) return;
    if (s->h3)      quiche_h3_conn_free(s->h3);
    if (s->conn)    quiche_conn_free(s->conn);
    if (s->h3_cfg)  quiche_h3_config_free(s->h3_cfg);
    if (s->cfg)     quiche_config_free(s->cfg);
    if (s->sock > 0) close(s->sock);
    efree(s);
}

static const zend_function_entry quicpro_funcs[] = {
    PHP_FE(quicpro_connect,                arginfo_quicpro_connect)
    PHP_FE(quicpro_close,                  arginfo_quicpro_close)
    PHP_FE(quicpro_send_request,           arginfo_quicpro_send_request)
    PHP_FE(quicpro_receive_response,       arginfo_quicpro_receive_response)
    PHP_FE(quicpro_poll,                   arginfo_quicpro_poll)
    PHP_FE(quicpro_cancel_stream,          arginfo_quicpro_cancel_stream)
    PHP_FE(quicpro_export_session_ticket,  arginfo_quicpro_export_session_ticket)
    PHP_FE(quicpro_import_session_ticket,  arginfo_quicpro_import_session_ticket)
    PHP_FE(quicpro_set_ca_file,            arginfo_quicpro_set_ca_file)
    PHP_FE(quicpro_set_client_cert,        arginfo_quicpro_set_client_cert)
    PHP_FE(quicpro_get_last_error,         arginfo_quicpro_get_last_error)
    PHP_FE(quicpro_get_stats,              arginfo_quicpro_get_stats)
    PHP_FE(quicpro_version,                arginfo_quicpro_version)
    PHP_FE_END
};

zend_module_entry quicpro_async_module_entry = {
    STANDARD_MODULE_HEADER,
    "quicpro_async",
    quicpro_funcs,
    PHP_MINIT(quicpro_async),
    NULL, NULL, NULL,
    PHP_MINFO(quicpro_async),
    PHP_QUICPRO_VERSION,
    STANDARD_MODULE_PROPERTIES
};

ZEND_GET_MODULE(quicpro_async)

PHP_MINIT_FUNCTION(quicpro_async)
{
#ifdef PHP_WIN32
    WSADATA w; WSAStartup(MAKEWORD(2,2), &w);
#endif
    le_quicpro = zend_register_list_destructors_ex(
        quicpro_session_dtor, NULL, "quicpro", module_number);
    return SUCCESS;
}

PHP_MINFO_FUNCTION(quicpro_async)
{
    php_info_print_table_start();
    php_info_print_table_row(2, "quicpro_async support", "enabled");
    php_info_print_table_row(2, "libquiche version", quiche_version());
    php_info_print_table_end();
}
