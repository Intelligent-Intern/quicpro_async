#ifndef PHP_QUICPRO_H
#define PHP_QUICPRO_H

#include <php.h>
#include <quiche.h>
#include <stdbool.h>

typedef struct {
    int               sock;
    quiche_config    *cfg;
    quiche_conn      *conn;
    quiche_h3_config *h3_cfg;
    quiche_h3_conn   *h3;
} quicpro_session_t;

extern int le_quicpro;

PHP_MINIT_FUNCTION(quicpro_async);
PHP_MSHUTDOWN_FUNCTION(quicpro_async);
PHP_MINFO_FUNCTION(quicpro_async);

PHP_FUNCTION(quicpro_connect);
PHP_FUNCTION(quicpro_close);
PHP_FUNCTION(quicpro_send_request);
PHP_FUNCTION(quicpro_receive_response);
PHP_FUNCTION(quicpro_poll);

#endif