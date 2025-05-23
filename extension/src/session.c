/*
 * session.c  –  Session management for php-quicpro_async
 *
 * A “session” in this context is a live QUIC connection that PHP code
 * can use to perform multiple HTTP/3 requests over the same connection.
 * We represent this in C with the `quicpro_session_t` struct (defined in
 * session.h), wrap it in a Zend object so PHP’s GC can manage its lifetime,
 * and expose methods like __construct() and close() on the PHP class
 * Quicpro\Session.
 *
 * Responsibilities:
 *   • Allocate and initialize a new QUIC session (UDP socket, QUIC handshake,
 *     HTTP/3 context).
 *   • Wrap the native pointer in a Zend object, so PHP can pass it around.
 *   • Clean up (close socket, free quiche resources) when the PHP object
 *     is garbage-collected or explicitly closed.
 *
 * Why sessions?
 *   QUIC is a connection-oriented protocol.  If you want to do multiple
 *   HTTP/3 requests efficiently, you keep a single QUIC connection (“session”)
 *   open and multiplex streams over it.  This file manages those sessions.
 */

#include "session.h"               /* quicpro_session_t definition */
#include "php_quicpro.h"           /* core extension definitions */
#include "php_quicpro_arginfo.h"   /* PHP_ARGINFO for methods */
#include <Zend/zend_exceptions.h>   /* zend_throw_exception() */
#include <quiche.h>                /* quiche_connect, quiche_h3_* APIs */
#include <openssl/rand.h>          /* RAND_bytes() for generating connection IDs */
#include <errno.h>                 /* errno for socket errors */
#include <netdb.h>                 /* getaddrinfo() */
#include <netinet/in.h>            /* sockaddr_in, sockaddr_in6 */
#include <unistd.h>                /* close() */
#include <sys/socket.h>            /* socket(), connect() */
#include <string.h>                /* memcpy(), strncpy(), memset() */

/* ---------------------------------------------------------------------------
 * Forward declaration of helper in another translation unit:
 *   quicpro_fetch_config(zval *zcfg)
 *
 * This function retrieves a `quiche_config*` from a PHP resource, or NULL
 * if the resource is invalid.  We need it below in the constructor to get
 * the TLS + QUIC parameters.
 */
extern quiche_config *quicpro_fetch_config(zval *zcfg);

/* ---------------------------------------------------------------------------
 * Optional: Extern symbol for legacy resource management
 */
extern int le_quicpro;

/* ──────────────────────────────────────────────────────────────────────────
 * Zend object handlers for Quicpro\Session
 *
 * We need custom handlers so that when the PHP object is destroyed,
 * we free our native quicpro_session_t and all its internal resources.
 * zend_object_handlers is a table of function pointers that define
 * object creation, cloning, comparison, property access, and destruction.
 * Here, we only override `free_obj`.
 *───────────────────────────────────────────────────────────────────────────*/
static zend_object_handlers quicpro_session_handlers;

/*
 * quicpro_session_free_obj()
 *
 * Called by PHP’s GC when the last reference to a Quicpro\Session object
 * is gone.  We must:
 *   1) Tear down the HTTP/3 context (quiche_h3_conn_free)
 *   2) Tear down the QUIC connection (quiche_conn_free)
 *   3) Tear down the HTTP/3 config (quiche_h3_config_free)
 *   4) Tear down the QUIC config (quiche_config_free)
 *   5) Close the underlying UDP socket
 *   6) Free the allocated quicpro_session_t struct
 *   7) Call the standard Zend destructor to free object properties.
 */
static void quicpro_session_free_obj(zend_object *zobj)
{
    /* Recover our wrapper struct from the zend_object pointer */
    quicpro_session_object *intern =
        php_quicpro_obj_from_zend(zobj);
    quicpro_session_t *s = intern->sess;

    if (s) {
        /* Free HTTP/3 connection state if it exists */
        if (s->h3) {
            quiche_h3_conn_free(s->h3);
            s->h3 = NULL;
        }
        /* Free HTTP/3 configuration */
        if (s->h3_cfg) {
            quiche_h3_config_free(s->h3_cfg);
            s->h3_cfg = NULL;
        }
        /* Free the QUIC connection object */
        if (s->conn) {
            quiche_conn_free(s->conn);
            s->conn = NULL;
        }
        /* Free the QUIC configuration (immutable after use) */
        if (s->cfg) {
            quiche_config_free((quiche_config *)s->cfg);
            s->cfg = NULL;
        }
        /* Close the UDP socket if open */
        if (s->sock >= 0) {
            close(s->sock);
            s->sock = -1;
        }
        /* Finally, free the C struct itself */
        efree(s);
        intern->sess = NULL;
    }

    /* Let Zend clean up the object’s properties, GC table, etc. */
    zend_object_std_dtor(&intern->std);
}

/* ──────────────────────────────────────────────────────────────────────────
 * quicpro_session_create()
 *
 * This is the `create_object` handler for the class Quicpro\Session.  PHP
 * calls this when you do `new Quicpro\Session(...)`.  We must:
 *   1) Allocate our combined wrapper struct (quicpro_session_object).
 *   2) Initialize the Zend object header.
 *   3) Install our free_obj handler.
 *   4) Set the wrapper’s `sess` pointer to NULL; it will be set in __construct().
 *   5) Return the zend_object* to the engine.
 *───────────────────────────────────────────────────────────────────────────*/
static zend_object *quicpro_session_create(zend_class_entry *ce)
{
    /* Allocate space for our custom struct and the standard Zend object props */
    quicpro_session_object *intern =
        ecalloc(1, sizeof(*intern) + zend_object_properties_size(ce));

    /* Standard initialization of Zend object header */
    zend_object_std_init(&intern->std, ce);
    object_properties_init(&intern->std, ce);

    /* On first run, copy the std handlers and override free_obj */
    if (!quicpro_session_handlers.free_obj) {
        memcpy(&quicpro_session_handlers,
               zend_get_std_object_handlers(),
               sizeof(zend_object_handlers));
        quicpro_session_handlers.offset   = XtOffsetOf(quicpro_session_object, std);
        quicpro_session_handlers.free_obj = quicpro_session_free_obj;
    }

    /* Use our handlers for this object */
    intern->std.handlers = &quicpro_session_handlers;

    /* Native session pointer not yet created (will be in __construct) */
    intern->sess = NULL;

    /* Return the zend_object pointer back to PHP */
    return &intern->std;
}

/* ──────────────────────────────────────────────────────────────────────────
 * Argument information for Quicpro\Session methods
 *
 * We declare, for PHP’s reflection and argument parsing, how many and what
 * types of parameters each method expects.
 *───────────────────────────────────────────────────────────────────────────*/
ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_session_construct, 0, 0, 3)
    ZEND_ARG_TYPE_INFO(0, host, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, port, IS_LONG,   0)
    ZEND_ARG_INFO(0, config)    /* resource quicpro_config */
    ZEND_ARG_TYPE_INFO(0, numa_node, IS_LONG, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_session_close, 0, 0, 0)
ZEND_END_ARG_INFO()

/* ──────────────────────────────────────────────────────────────────────────
 * PHP Method: Quicpro\Session::__construct(string $host, int $port,
 *                                         resource $cfg [, int $numa_node])
 *
 * Steps:
 *   1) Parse PHP parameters from zval stack.
 *   2) Allocate native quicpro_session_t and initialize defaults:
 *      - Store the host (for SNI/authority)
 *      - Generate a random source connection ID
 *      - Mark socket as invalid (-1)
 *   3) Fetch the quiche_config* from the PHP resource.
 *      If invalid, free and throw an exception.
 *   4) Resolve DNS via getaddrinfo(), trying both IPv6 and IPv4.
 *   5) Create a non-blocking UDP socket and connect() to the first
 *      successful address.
 *   6) Call quiche_connect() to begin the QUIC handshake.
 *   7) Initialize HTTP/3 context with quiche_h3_config_new()
 *      and quiche_h3_conn_new_with_transport().
 *   8) Store the filled-in quicpro_session_t in the PHP object.
 *───────────────────────────────────────────────────────────────────────────*/
PHP_METHOD(QuicSession, __construct)
{
    char       *host;
    size_t      host_len;
    zend_long   port;
    zval       *zcfg;
    zend_long   numa_node = -1;

    /* 1) Parse parameters: host (string), port (int), config (resource), optional numa_node */
    ZEND_PARSE_PARAMETERS_START(3, 4)
        Z_PARAM_STRING(host, host_len)
        Z_PARAM_LONG(port)
        Z_PARAM_RESOURCE(zcfg)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(numa_node)
    ZEND_PARSE_PARAMETERS_END();

    /* Prepare the PHP object’s internal wrapper */
    quicpro_session_object *intern =
        php_quicpro_obj_from_zend(Z_OBJ_P(getThis()));
    /* Allocate and zero our session struct */
    quicpro_session_t *s = ecalloc(1, sizeof(*s));
    /* 2a) Copy host for later SNI/authority header */
    strncpy(s->host, host, sizeof(s->host) - 1);
    /* 2b) Remember NUMA node if provided (for future optimizations) */
    s->numa_node = (int)numa_node;
    /* 2c) Mark socket as closed until we create it */
    s->sock      = -1;
    /* 2d) Generate a random source connection ID for QUIC */
    RAND_bytes(s->scid, sizeof(s->scid));

    /* 3) Fetch the QUIC config resource (immutable after use) */
    quiche_config *cfg = quicpro_fetch_config(zcfg);
    if (!cfg) {
        /* Invalid config resource: clean up and throw */
        efree(s);
        zend_throw_exception(zend_exception_get_default(),
                             "Invalid QUIC configuration resource", 0);
        return;
    }
    s->cfg = cfg;

    /* 4) Resolve the DNS name into sockaddr structures */
    struct addrinfo hints = {
        .ai_family   = AF_UNSPEC,       /* IPv6 + IPv4 */
        .ai_socktype = SOCK_DGRAM,      /* UDP */
        .ai_flags    = AI_ADDRCONFIG,   /* only return configured families */
    };
    struct addrinfo *ai_list = NULL;
    char port_str[6];
    snprintf(port_str, sizeof(port_str), "%ld", port);

    if (getaddrinfo(host, port_str, &hints, &ai_list) != 0) {
        efree(s);
        zend_throw_exception(zend_exception_get_default(),
                             "DNS resolution failed", 0);
        return;
    }

    /* 5) Loop addresses, create non-blocking UDP socket, connect() */
    struct addrinfo *it;
    struct sockaddr_storage peer;
    socklen_t peer_len = 0;
    for (it = ai_list; it; it = it->ai_next) {
        int fd = socket(it->ai_family, SOCK_DGRAM | SOCK_NONBLOCK, 0);
        if (fd < 0) {
            continue;  /* try next address */
        }
        if (connect(fd, it->ai_addr, it->ai_addrlen) == 0) {
            s->sock = fd;
            memcpy(&peer, it->ai_addr, it->ai_addrlen);
            peer_len = (socklen_t)it->ai_addrlen;
            break;    /* connected successfully */
        }
        close(fd);
    }
    freeaddrinfo(ai_list);

    if (s->sock < 0) {
        efree(s);
        zend_throw_exception(zend_exception_get_default(),
                             "Unable to connect UDP socket", 0);
        return;
    }

    /* 6) Start QUIC handshake; returns quiche_conn* or NULL on failure */
    s->conn = quiche_connect(host,
                             (const uint8_t *)s->scid, sizeof(s->scid),
                             NULL, 0,
                             (const struct sockaddr *)&peer, peer_len,
                             cfg);
    if (!s->conn) {
        close(s->sock);
        efree(s);
        zend_throw_exception(zend_exception_get_default(),
                             "quiche_connect() failed", 0);
        return;
    }

    /* 7) Initialize HTTP/3 on top of our QUIC transport */
    s->h3_cfg = quiche_h3_config_new();
    s->h3     = quiche_h3_conn_new_with_transport(s->conn, s->h3_cfg);
    if (!s->h3) {
        quiche_conn_free(s->conn);
        close(s->sock);
        efree(s);
        zend_throw_exception(zend_exception_get_default(),
                             "HTTP/3 initialization failed", 0);
        return;
    }

    /* 8) All set — store our native session pointer in the PHP object */
    intern->sess = s;
}

/* ──────────────────────────────────────────────────────────────────────────
 * PHP Method: Quicpro\Session::close(): bool
 *
 * Gracefully close the QUIC connection, sending the “FIN” and closing
 * underlying socket.  Returns true on success, false if already closed.
 *───────────────────────────────────────────────────────────────────────────*/
PHP_METHOD(QuicSession, close)
{
    /* Fetch our native session pointer from $this */
    quicpro_session_t *s = quicpro_obj_fetch(getThis());
    if (!s || !s->conn) {
        /* Nothing to close */
        RETURN_FALSE;
    }

    /* Tell quiche to send a connection-close frame */
    quiche_conn_close(s->conn,
                      true,            /* immediate close? */
                      0,               /* application error code */
                      (const uint8_t *)"kthxbye", 7 /* reason phrase */);
    /* We leave the socket open until free_obj runs to keep the final frames */
    RETURN_TRUE;
}

/* ──────────────────────────────────────────────────────────────────────────
 * Global function: quicpro_close() – For legacy/resource usage
 *
 * Allows closing a session from PHP with quicpro_close($session).
 * Supports both OOP and resource handles.
 * Returns true if connection closed, false if already closed or invalid.
 *───────────────────────────────────────────────────────────────────────────*/
PHP_FUNCTION(quicpro_close)
{
    zval *zsession;
    quicpro_session_t *s = NULL;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_RESOURCE(zsession)
    ZEND_PARSE_PARAMETERS_END();

    if (Z_TYPE_P(zsession) == IS_OBJECT) {
        s = quicpro_obj_fetch(zsession);
    } else if (Z_TYPE_P(zsession) == IS_RESOURCE) {
        s = (quicpro_session_t *)zend_fetch_resource_ex(zsession, "Quicpro\\Session", le_quicpro);
    }

    if (!s || !s->conn) {
        RETURN_FALSE;
    }

    quiche_conn_close(
        s->conn,
        true, // application close
        0,
        (const uint8_t *)"kthxbye", 7
    );

    RETURN_TRUE;
}

/* ──────────────────────────────────────────────────────────────────────────
 * Method table and class registration
 *
 * We tell PHP “Quicpro\Session” has these methods, with these arginfos,
 * and that it uses quicpro_session_create() for instantiation.
 *───────────────────────────────────────────────────────────────────────────*/
static const zend_function_entry quicpro_session_methods[] = {
    PHP_ME(QuicSession, __construct, arginfo_quicpro_session_construct, ZEND_ACC_PUBLIC)
    PHP_ME(QuicSession, close,       arginfo_quicpro_session_close,   ZEND_ACC_PUBLIC)
    PHP_FE_END
};

/* Global pointer to the class entry, set in MINIT elsewhere */
zend_class_entry *quicpro_ce_session;

/*
 * quicpro_register_session_ce()
 *
 * Called during module initialization to register the PHP class:
 *   class Quicpro\Session { public function __construct(...); public function close(): bool; }
 * and to hook up our custom object handlers.
 */
void quicpro_register_session_ce(void)
{
    zend_class_entry ce;
    INIT_CLASS_ENTRY(ce, "Quicpro\\Session", quicpro_session_methods);
    quicpro_ce_session = zend_register_internal_class(&ce);
    quicpro_ce_session->create_object = quicpro_session_create;
}
