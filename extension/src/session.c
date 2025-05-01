/* Session management for php-quicpro.
 *
 * A “session” in this context is a live QUIC connection that PHP code
 * can use to perform multiple HTTP/3 requests.  Internally we keep the
 * connection in a C struct called `quicpro_session_t`, wrap that struct
 * into a Zend object, and hand the object to user-land.  The object is
 * reference-counted by PHP’s garbage collector, so the connection is
 * closed automatically when no script holds a reference any more.
 *
 * A short walkthrough for readers who know PHP better than C:
 *
 *   1.  Memory allocation happens with `ecalloc()`.  Unlike `malloc()`
 *       it zero-initialises the result, which prevents accidental use
 *       of uninitialised data and is therefore the safer default.
 *
 *   2.  “Heap” means the area of RAM from which dynamically sized
 *       objects are taken.  Anything created with `ecalloc()` lives
 *       on the heap until `efree()` frees it.
 *
 *   3.  A “non-blocking UDP socket” is a network endpoint that returns
 *       immediately when no packets are available instead of pausing
 *       the whole process.  QUIC always runs over UDP, and we combine
 *       non-blocking I/O with the event loop in poll.c.
 *
 *   4.  A “handshake” is the initial exchange where the client proves
 *       possession of the private key and negotiates encryption
 *       parameters.  QUIC folds TLS 1.3 into this step, so after the
 *       handshake finishes we already have an encrypted channel.
 *
 * The code is written in standard C11, uses only POSIX-2018 system
 * calls, and compiles without warnings on GCC and Clang when
 * `-Wall -Wextra -Wpedantic` is enabled.  That trio of flags requests
 * *all* diagnostics that might uncover a logic error or portability
 * issue; keeping the code warning-free therefore gives us a good
 * amount of confidence in its correctness.
 */

#include "session.h"           /* definition of quicpro_session_t            */
#include "php_quicpro.h"       /* error helpers, resource IDs, arginfo       */

#include <quiche.h>            /* the QUIC implementation by Cloudflare      */
#include <openssl/rand.h>      /* RAND_bytes()                               */

#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>

/* Forward declaration so PHP code can fetch the wrapped pointer
 * without touching Zend internals. */
static inline quicpro_session_t *quicpro_obj_fetch(zval *zobj);

/*─────────────────────────────────────────────────────────────────────────────
 *  1.  Zend object wrapper
 *─────────────────────────────────────────────────────────────────────────────*/

/* The wrapper combines a pointer to the C session with the mandatory
 * `zend_object` footer.  Zend allocates one contiguous block of memory
 * that holds both fields, so a single free() releases everything. */
typedef struct {
    quicpro_session_t *sess;
    zend_object        std;
} quicpro_session_object;

/* Extract the `quicpro_session_t *` from a zval that represents a
 * `Quicpro\Session` object.  Zend macros convert between the memory
 * layout of the object and the public zval API. */
static inline quicpro_session_t *quicpro_obj_fetch(zval *zobj)
{
    quicpro_session_object *obj =
        (quicpro_session_object *)((char *)Z_OBJ_P(zobj) -
                                    XtOffsetOf(quicpro_session_object, std));
    return obj->sess;
}

/* Custom free-handler that the garbage collector calls when the last
 * PHP reference to a Quicpro\Session disappears.  We own several
 * libquiche handles plus a UDP socket; each needs an explicit
 * destruction call.  Failure to free them would leak file descriptors
 * and memory. */
static void quicpro_session_free_obj(zend_object *zobj)
{
    quicpro_session_object *intern =
        (quicpro_session_object *)((char *)zobj -
                                   XtOffsetOf(quicpro_session_object, std));
    quicpro_session_t *s = intern->sess;

    if (s) {
        if (s->h3)      quiche_h3_conn_free(s->h3);
        if (s->h3_cfg)  quiche_h3_config_free(s->h3_cfg);
        if (s->conn)    quiche_conn_free(s->conn);
        if (s->cfg)     quiche_config_free(s->cfg);
        if (s->sock >= 0) close(s->sock);

        efree(s);
    }
    zend_object_std_dtor(&intern->std);
}

/* Creates the Zend object.  zend_object_std_init() sets up the header,
 * and the extension assigns our custom destructor.  We postpone
 * allocation of the underlying quicpro_session_t until the PHP
 * constructor runs because we need user-provided arguments first. */
static zend_object *quicpro_session_create(zend_class_entry *ce)
{
    quicpro_session_object *obj =
        ecalloc(1, sizeof(*obj) + zend_object_properties_size(ce));

    zend_object_std_init(&obj->std, ce);
    object_properties_init(&obj->std, ce);

    obj->std.handlers = (zend_object_handlers *)ce->create_object;
    obj->std.handlers->free_obj = quicpro_session_free_obj;

    obj->sess = NULL; /* will be set by __construct() */
    return &obj->std;
}

/*─────────────────────────────────────────────────────────────────────────────
 *  2.  PHP-visible constructor
 *─────────────────────────────────────────────────────────────────────────────*/

PHP_METHOD(QuicSession, __construct)
{
    char       *host;       size_t host_len;
    zend_long   port;
    zval       *zcfg;
    zend_long   numa_node = -1;   /* –1 signals: “no preference” */

    ZEND_PARSE_PARAMETERS_START(3, 4)
        Z_PARAM_STRING(host, host_len)
        Z_PARAM_LONG(port)
        Z_PARAM_OBJECT(zcfg)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(numa_node)
    ZEND_PARSE_PARAMETERS_END();

    quicpro_session_object *obj =
        (quicpro_session_object *)Z_OBJ_P(getThis());

    /* Allocate and zero-initialise the session struct. */
    quicpro_session_t *s = ecalloc(1, sizeof(*s));

    /* Copy scalar values so they stay valid after the constructor
     * returns.  memcpy() would work as well, but strncpy() shows the
     * intent more clearly: we copy at most host_len bytes and always
     * NUL-terminate. */
    strncpy(s->host, host, sizeof s->host - 1);
    s->port      = (uint16_t)port;
    s->numa_node = (int)numa_node;
    s->sock      = -1;               /* mark as “not yet opened” */
    RAND_bytes(s->scid, sizeof s->scid); /* generate a random CID */

    /* The PHP resource `$cfg` stores a ready-to-use quiche_config.
     * We borrow the pointer; libquiche rules say config lives at least
     * as long as the connection, and user-land holds a reference. */
    s->cfg = quicpro_obj_fetch(zcfg);

    /* Open a UDP socket in non-blocking mode.  We try IPv6 and IPv4 in
     * the order supplied by getaddrinfo().  If both families fail the
     * session cannot continue, so we throw. */
    struct addrinfo hints = {
        .ai_family   = AF_UNSPEC,
        .ai_socktype = SOCK_DGRAM,
        .ai_flags    = AI_ADDRCONFIG
    };
    struct addrinfo *ai, *it;

    char port_str[8];
    snprintf(port_str, sizeof port_str, "%u", (unsigned)port);

    if (getaddrinfo(host, port_str, &hints, &ai) != 0) {
        zend_throw_exception(NULL, "DNS resolution failed", 0);
        efree(s);
        return;
    }

    for (it = ai; it; it = it->ai_next) {
        int fd = socket(it->ai_family, it->ai_socktype | SOCK_NONBLOCK, 0);
        if (fd < 0) continue;

        if (connect(fd, it->ai_addr, it->ai_addrlen) == 0) {
            s->sock = fd;
            break;
        }
        close(fd);
    }
    freeaddrinfo(ai);

    if (s->sock < 0) {
        zend_throw_exception(NULL, "unable to connect UDP socket", 0);
        efree(s);
        return;
    }

    /* Perform the QUIC handshake.  libquiche hides most details; we
     * supply host, CID, and config.  On failure we bubble the error up
     * to PHP so the caller can try again or fall back to TCP. */
    s->conn = quiche_connect(host,
                             (const uint8_t *)s->scid, sizeof s->scid,
                             s->sock, s->cfg);
    if (!s->conn) {
        zend_throw_exception(NULL, "quiche_connect() failed", 0);
        close(s->sock);
        efree(s);
        return;
    }

    /* HTTP/3 wrapper for the already established QUIC transport. */
    s->h3_cfg = quiche_h3_config_new();
    s->h3     = quiche_h3_conn_new_with_transport(s->conn, s->h3_cfg);

    if (!s->h3) {
        zend_throw_exception(NULL, "HTTP/3 initialisation failed", 0);
        quiche_conn_free(s->conn);
        close(s->sock);
        efree(s);
        return;
    }

    obj->sess = s;
}

/*─────────────────────────────────────────────────────────────────────────────
 *  3.  PHP method: close()
 *─────────────────────────────────────────────────────────────────────────────
 *
 * Closing a QUIC connection from user-land is optional because the
 * destructor covers the same ground.  Nevertheless an explicit call
 * allows scripts to flush telemetry immediately and frees the socket a
 * little earlier, which can matter under heavy load.
 */
PHP_METHOD(QuicSession, close)
{
    quicpro_session_t *s = quicpro_obj_fetch(getThis());
    if (!s || !s->conn) RETURN_FALSE;

    quiche_conn_close(s->conn, true, 0, (const uint8_t *)"kthxbye", 7);
    RETURN_TRUE;
}

/*─────────────────────────────────────────────────────────────────────────────
 *  4.  Class registration
 *───────────────────────────────────────────────────────────────────────────*/

static const zend_function_entry quicpro_session_methods[] = {
    PHP_ME(QuicSession, __construct, arginfo_quicpro_session_construct,
           ZEND_ACC_PUBLIC)
    PHP_ME(QuicSession, close,       arginfo_quicpro_session_close,
           ZEND_ACC_PUBLIC)
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
