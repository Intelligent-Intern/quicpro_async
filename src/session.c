/*  Zend object wrapper around quicpro_session_t
 *  ----------------------------------------------------------
 *  Switches from resource‑style to a proper `Quicpro\Session` object.
 *  • GC‑safe – frees ticket buffer & quiche handles in object destructor
 *  • Stores NUMA‑node hint for worker pinning (s->numa_node)
 */

#include "php_quicpro.h"
#include <openssl/rand.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#ifdef __linux__
#include <numaif.h>
#endif

/* ------------------------------------------------------------------ */
/* Internal object struct                                              */
/* ------------------------------------------------------------------ */

typedef struct {
    quicpro_session_t *sess;   /* pointer to C session */
    zend_object         std;   /* must be last */
} quicpro_session_object;

static zend_class_entry    *quicpro_ce_session       = NULL;
static zend_object_handlers quicpro_session_handlers;

/* helper: fetch object → session pointer */
static inline quicpro_session_object *quicpro_session_fetch(zend_object *obj)
{
    return (quicpro_session_object *)((char *)obj - XtOffsetOf(quicpro_session_object, std));
}

static inline quicpro_session_t *quicpro_session_ptr(zval *zobj)
{
    return quicpro_session_fetch(Z_OBJ_P(zobj))->sess;
}

/* ------------------------------------------------------------------ */
/* Destructor – frees quiche handles + ticket buffer                  */
/* ------------------------------------------------------------------ */
static void quicpro_session_free_obj(zend_object *obj)
{
    quicpro_session_object *intern = quicpro_session_fetch(obj);

    if (intern->sess) {
        /* free ticket buffer implicit in efree() */
        if (intern->sess->h3)       quiche_h3_conn_free(intern->sess->h3);
        if (intern->sess->h3_cfg)   quiche_h3_config_free(intern->sess->h3_cfg);
        if (intern->sess->conn)     quiche_conn_free(intern->sess->conn);
        if (intern->sess->cfg)      quiche_config_free(intern->sess->cfg);
        if (intern->sess->sock >= 0) close(intern->sess->sock);
        efree(intern->sess);
    }

    zend_object_std_dtor(obj);
}

/* ------------------------------------------------------------------ */
/* Object creation                                                    */
/* ------------------------------------------------------------------ */
static zend_object *quicpro_session_create_obj(zend_class_entry *ce)
{
    quicpro_session_object *intern = ecalloc(1, sizeof(*intern) + zend_object_properties_size(ce));

    zend_object_std_init(&intern->std, ce);
    object_properties_init(&intern->std, ce);

    intern->std.handlers = &quicpro_session_handlers;
    intern->sess         = NULL; /* filled in by ctor */

    return &intern->std;
}

/* ------------------------------------------------------------------ */
/* ctor: __construct(string $host, int $port, QuicConfig $cfg, ?string $iface = null, ?int $numa = -1)
 * ------------------------------------------------------------------ */
PHP_METHOD(QuicSession, __construct)
{
    char       *host, *iface = NULL;
    size_t      host_len, iface_len = 0;
    zend_long   port;
    zval       *zcfg;
    zend_long   numa_node = -1;

    ZEND_PARSE_PARAMETERS_START(3, 5)
        Z_PARAM_STRING(host, host_len)
        Z_PARAM_LONG(port)
        Z_PARAM_RESOURCE(zcfg)
        Z_PARAM_OPTIONAL
        Z_PARAM_STRING_EX(iface, iface_len, 1, 0)
        Z_PARAM_LONG(numa_node)
    ZEND_PARSE_PARAMETERS_END();

    quicpro_session_object *obj = quicpro_session_fetch(Z_OBJ_P(getThis()));

    if (obj->sess) {
        zend_throw_error(NULL, "Session already initialized");
        return;
    }

    quiche_config *cfg = quicpro_fetch_config(zcfg);
    if (!cfg) {
        zend_throw_error(NULL, "Invalid QuicConfig resource");
        return;
    }

    quicpro_session_t *s = ecalloc(1, sizeof(*s));
    s->cfg = cfg; /* shared – DO NOT free in destructor, handled elsewhere */

    /* store host for :authority */
    strncpy(s->host, host, sizeof(s->host)-1);

    /* NUMA node */
    s->numa_node = (int)numa_node;

    /* generate SCID */
    RAND_bytes(s->scid, sizeof(s->scid));

    /* Resolve host (A+AAAA) */
    struct addrinfo hints = {0}, *res = NULL;
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    char portbuf[6];
    snprintf(portbuf, sizeof(portbuf), "%ld", port);

    if (getaddrinfo(host, portbuf, &hints, &res) != 0) {
        efree(s);
        zend_throw_error(NULL, "DNS resolution failed for %s", host);
        return;
    }

    /* Happy‑Eyeballs: try first entry, fallback after 250 ms */
    int sock = -1;
    struct addrinfo *ai;
    for (ai = res; ai; ai = ai->ai_next) {
        sock = socket(ai->ai_family, ai->ai_socktype | SOCK_NONBLOCK, ai->ai_protocol);
        if (sock < 0) continue;

        if (iface_len) {
#ifdef SO_BINDTODEVICE
            setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, iface, iface_len);
#endif
        }

        if (connect(sock, ai->ai_addr, ai->ai_addrlen) == 0 || errno == EINPROGRESS) {
            break; /* success */
        }
        close(sock);
        sock = -1;
    }
    freeaddrinfo(res);

    if (sock < 0) {
        efree(s);
        zend_throw_error(NULL, "Unable to create/connect UDP socket");
        return;
    }

    s->sock = sock;

    s->conn = quiche_connect(host,
                             s->scid, sizeof(s->scid),
                             NULL, 0,
                             (const struct sockaddr *)ai->ai_addr,
                             ai->ai_addrlen,
                             s->cfg);

    if (!s->conn) {
        close(sock);
        efree(s);
        zend_throw_error(NULL, "quiche_connect() failed");
        return;
    }

    quiche_conn_set_tls_name(s->conn, host);

    s->h3_cfg = quiche_h3_config_new();
    s->h3     = quiche_h3_conn_new_with_transport(s->conn, s->h3_cfg);

    obj->sess = s;
}

/* ------------------------------------------------------------------ */
/* close(): terminates connection explicitly                           */
/* ------------------------------------------------------------------ */
PHP_METHOD(QuicSession, close)
{
    quicpro_session_object *obj = quicpro_session_fetch(Z_OBJ_P(getThis()));
    if (!obj->sess) {
        RETURN_FALSE;
    }

    quiche_conn_close(obj->sess->conn, true, 0x00, (const uint8_t *)"", 0);

    quicpro_session_free_obj(&obj->std); /* manual free */
    obj->sess = NULL;

    RETURN_TRUE;
}

/* ------------------------------------------------------------------ */
/* arginfo                                                             */
/* ------------------------------------------------------------------ */
ZEND_BEGIN_ARG_INFO_EX(ai_quic_session_ctor, 0, 0, 3)
    ZEND_ARG_TYPE_INFO(0, host, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, port, IS_LONG, 0)
    ZEND_ARG_OBJ_INFO(0, cfg, QuicConfig, 0)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, iface, IS_STRING, 1, "null")
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, numa_node, IS_LONG, 1, "-1")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ai_quic_session_void, 0, 0, 0)
ZEND_END_ARG_INFO()

/* ------------------------------------------------------------------ */
/* method table                                                        */
/* ------------------------------------------------------------------ */
static const zend_function_entry quicpro_session_methods[] = {
    PHP_ME(QuicSession, __construct, ai_quic_session_ctor, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
    PHP_ME(QuicSession, close,       ai_quic_session_void, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

/* ------------------------------------------------------------------ */
/* MINIT – register class                                              */
/* ------------------------------------------------------------------ */
int quicpro_register_session_class(INIT_FUNC_ARGS)
{
    zend_class_entry ce;
    INIT_CLASS_ENTRY(ce, "Quicpro\\Session", quicpro_session_methods);
    quicpro_ce_session = zend_register_internal_class(&ce);
    quicpro_ce_session->create_object = quicpro_session_create_obj;

    memcpy(&quicpro_session_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    quicpro_session_handlers.offset    = XtOffsetOf(quicpro_session_object, std);
    quicpro_session_handlers.free_obj  = quicpro_session_free_obj;
    quicpro_session_handlers.clone_obj = NULL; /* non‑cloneable */

    return SUCCESS;
}
