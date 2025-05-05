/*
 * Session management for php-quicpro_async.
 *
 * A “session” in this context is a live QUIC connection that PHP code
 * can use to perform multiple HTTP/3 requests.  Internally we keep the
 * connection in a C struct called `quicpro_session_t`, wrap that struct
 * into a Zend object, and hand the object to user-land.  The object is
 * reference-counted by PHP’s garbage collector, so the connection is
 * closed automatically when no script holds a reference any more.
 */

#include "session.h"               /* quicpro_session_t definition */
#include "php_quicpro.h"           /* core headers, error helpers */
#include "php_quicpro_arginfo.h"   /* method arginfo */
#include <Zend/zend_exceptions.h>   /* zend_throw_exception */
#include <quiche.h>                /* QUIC implementation */
#include <openssl/rand.h>          /* RAND_bytes */
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>

/* Fetch a quiche_config* from a PHP resource */
extern quiche_config *quicpro_fetch_config(zval *zcfg);

/*─────────────────────────────────────────────────────────────────────────────
 * Zend object handlers for Quicpro\Session
 *─────────────────────────────────────────────────────────────────────────────*/
static zend_object_handlers quicpro_session_handlers;

/* Custom free-handler called when the last PHP reference to a Session disappears */
static void quicpro_session_free_obj(zend_object *zobj)
{
    quicpro_session_object *intern =
        (quicpro_session_object *)((char *)zobj -
            XtOffsetOf(quicpro_session_object, std));
    quicpro_session_t *s = intern->sess;

    if (s) {
        if (s->h3) {
            quiche_h3_conn_free(s->h3);
        }
        if (s->h3_cfg) {
            quiche_h3_config_free(s->h3_cfg);
        }
        if (s->conn) {
            quiche_conn_free(s->conn);
        }
        if (s->cfg) {
            quiche_config_free((quiche_config *)s->cfg);
        }
        if (s->sock >= 0) {
            close(s->sock);
        }
        efree(s);
    }

    zend_object_std_dtor(&intern->std);
}

/*─────────────────────────────────────────────────────────────────────────────
 * Create a new PHP Quicpro\Session object
 *─────────────────────────────────────────────────────────────────────────────*/
static zend_object *quicpro_session_create(zend_class_entry *ce)
{
    quicpro_session_object *obj =
        ecalloc(1, sizeof(*obj) + zend_object_properties_size(ce));

    zend_object_std_init(&obj->std, ce);
    object_properties_init(&obj->std, ce);

    if (!quicpro_session_handlers.free_obj) {
        memcpy(&quicpro_session_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
        quicpro_session_handlers.offset   = XtOffsetOf(quicpro_session_object, std);
        quicpro_session_handlers.free_obj = quicpro_session_free_obj;
    }

    obj->std.handlers = &quicpro_session_handlers;
    obj->sess         = NULL; /* set in __construct() */

    return &obj->std;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Argument information for Quicpro\Session methods
 *─────────────────────────────────────────────────────────────────────────────*/
ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_session_construct, 0, 0, 2)
    ZEND_ARG_TYPE_INFO(0, host, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, port, IS_LONG,   0)
    ZEND_ARG_INFO(0, config)
    ZEND_ARG_TYPE_INFO(0, numa_node, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_session_close, 0, 0, 0)
ZEND_END_ARG_INFO()

/*─────────────────────────────────────────────────────────────────────────────
 * PHP-visible constructor: __construct(string $host, int $port, resource $cfg [, int $numa_node])
 *─────────────────────────────────────────────────────────────────────────────*/
PHP_METHOD(QuicSession, __construct)
{
    char       *host;       size_t host_len;
    zend_long   port;
    zval       *zcfg;
    zend_long   numa_node = -1;

    ZEND_PARSE_PARAMETERS_START(3, 4)
        Z_PARAM_STRING(host, host_len)
        Z_PARAM_LONG(port)
        Z_PARAM_RESOURCE(zcfg)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(numa_node)
    ZEND_PARSE_PARAMETERS_END();

    quicpro_session_object *intern =
        (quicpro_session_object *)Z_OBJ_P(getThis());
    quicpro_session_t *s = ecalloc(1, sizeof(*s));

    /* Copy host for SNI/authority */
    strncpy(s->host, host, sizeof(s->host) - 1);

    s->numa_node = (int)numa_node;
    s->sock      = -1;
    RAND_bytes(s->scid, sizeof(s->scid));

    /* Fetch the QUIC config resource */
    quiche_config *cfg = quicpro_fetch_config(zcfg);
    if (!cfg) {
        efree(s);
        zend_throw_exception(zend_exception_get_default(), "Invalid config resource", 0);
        return;
    }
    s->cfg = cfg;

    /* Resolve address */
    struct addrinfo hints = {
        .ai_family   = AF_UNSPEC,
        .ai_socktype = SOCK_DGRAM,
        .ai_flags    = AI_ADDRCONFIG
    };
    struct addrinfo *ai_list = NULL, *it = NULL;
    char port_str[6];
    snprintf(port_str, sizeof(port_str), "%ld", port);

    if (getaddrinfo(host, port_str, &hints, &ai_list) != 0) {
        efree(s);
        zend_throw_exception(zend_exception_get_default(), "DNS resolution failed", 0);
        return;
    }

    struct sockaddr_storage peer;
    socklen_t peer_len = 0;

    for (it = ai_list; it; it = it->ai_next) {
        int fd = socket(it->ai_family, SOCK_DGRAM | SOCK_NONBLOCK, 0);
        if (fd < 0) continue;
        if (connect(fd, it->ai_addr, it->ai_addrlen) == 0) {
            s->sock = fd;
            memcpy(&peer, it->ai_addr, it->ai_addrlen);
            peer_len = it->ai_addrlen;
            break;
        }
        close(fd);
    }
    freeaddrinfo(ai_list);

    if (s->sock < 0) {
        efree(s);
        zend_throw_exception(zend_exception_get_default(), "Unable to connect UDP socket", 0);
        return;
    }

    /* Perform QUIC handshake */
    s->conn = quiche_connect(host,
                             (const uint8_t *)s->scid, sizeof(s->scid),
                             NULL, 0,
                             (const struct sockaddr *)&peer, peer_len,
                             cfg);

    if (!s->conn) {
        close(s->sock);
        efree(s);
        zend_throw_exception(zend_exception_get_default(), "quiche_connect() failed", 0);
        return;
    }

    /* Initialize HTTP/3 context */
    s->h3_cfg = quiche_h3_config_new();
    s->h3     = quiche_h3_conn_new_with_transport(s->conn, s->h3_cfg);

    if (!s->h3) {
        quiche_conn_free(s->conn);
        close(s->sock);
        efree(s);
        zend_throw_exception(zend_exception_get_default(), "HTTP/3 initialization failed", 0);
        return;
    }

    intern->sess = s;
}

/*─────────────────────────────────────────────────────────────────────────────
 * PHP method: close(): bool
 *─────────────────────────────────────────────────────────────────────────────*/
PHP_METHOD(QuicSession, close)
{
    quicpro_session_t *s = quicpro_obj_fetch(getThis());
    if (!s || !s->conn) {
        RETURN_FALSE;
    }

    quiche_conn_close(s->conn, true, 0, (const uint8_t *)"kthxbye", 7);
    RETURN_TRUE;
}

/*─────────────────────────────────────────────────────────────────────────────
 * Class registration for Quicpro\Session
 *─────────────────────────────────────────────────────────────────────────────*/
static const zend_function_entry quicpro_session_methods[] = {
    PHP_ME(QuicSession, __construct, arginfo_quicpro_session_construct, ZEND_ACC_PUBLIC)
    PHP_ME(QuicSession, close,       arginfo_quicpro_session_close,   ZEND_ACC_PUBLIC)
    PHP_FE_END
};

zend_class_entry *quicpro_ce_session;

void quicpro_register_session_ce(void)
{
    zend_class_entry ce;
    INIT_CLASS_ENTRY(ce, "Quicpro\\Session", quicpro_session_methods);
    quicpro_ce_session = zend_register_internal_class(&ce);
    quicpro_ce_session->create_object = quicpro_session_create;
}
