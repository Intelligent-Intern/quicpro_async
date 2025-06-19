/*
 * connect.c – Implementation of quicpro_connect() for php-quicpro_async
 * ---------------------------------------------------------------------
 * This file implements the logic to establish a QUIC session for userland PHP.
 * Features:
 *   - DNS resolution
 *   - Non-blocking UDP socket setup
 *   - Optional interface binding (SO_BINDTODEVICE)
 *   - Optional Happy Eyeballs logic (user-definable delay, or single-family)
 *   - QUIC handshake via quiche
 *   - Returns a PHP resource handle for the session
 */

#include <stddef.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <openssl/rand.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "php_quicpro.h"
#include "connect.h"
#include <quiche.h>

extern int le_quicpro; /* Registered resource ID for quicpro_session_t* */

/*
 * Bind a UDP socket to a specific network interface, if given.
 * Returns 0 on success, -1 on error.
 */
static int socket_bind_iface(int fd, const char *iface)
{
    return setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, iface, strlen(iface));
}

/*
 * Create a non-blocking UDP socket for the given address family.
 * Returns a valid socket fd, or -1 on error.
 */
static int udp_socket_nonblock(int family)
{
    return socket(family, SOCK_DGRAM | SOCK_NONBLOCK, 0);
}

/*
 * Resolve host:port to a linked list of address candidates.
 * Returns 0 on success, nonzero on error.
 */
static int resolve_host(const char *host, const char *port, struct addrinfo **res)
{
    struct addrinfo hints = {0};
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_family   = AF_UNSPEC;
    return getaddrinfo(host, port, &hints, res);
}

/*
 * PHP_FUNCTION(quicpro_connect)
 * -----------------------------
 * Establish a new QUIC/HTTP3 session. Supports Happy Eyeballs (RFC 8305).
 *
 * PHP signature:
 *   resource quicpro_connect(string $host, int $port, resource $cfg [, array $opts])
 *
 * $opts = [
 *    'mode'     => 'auto'|'ipv6'|'ipv4',  // default: auto
 *    'delay_ms' => <int>,                  // default: 250 (auto mode only)
 *    'iface'    => <string>                // optional: bind socket to interface
 * ]
 */
PHP_FUNCTION(quicpro_connect)
{
    char        *host;
    size_t       host_len;
    zend_long    port;
    zval        *zcfg;
    zval        *zopts = NULL;
    char        *iface = NULL;
    size_t       iface_len = 0;
    char        *mode = "auto";
    size_t       mode_len = 4;
    zend_long    delay_ms = 250;

    /* Parse arguments */
    ZEND_PARSE_PARAMETERS_START(3, 5)
        Z_PARAM_STRING(host, host_len)
        Z_PARAM_LONG(port)
        Z_PARAM_RESOURCE(zcfg)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY(zopts)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    /* Parse options array */
    if (zopts && Z_TYPE_P(zopts) == IS_ARRAY) {
        zval *z_mode   = zend_hash_str_find(Z_ARRVAL_P(zopts), "mode", sizeof("mode")-1);
        zval *z_delay  = zend_hash_str_find(Z_ARRVAL_P(zopts), "delay_ms", sizeof("delay_ms")-1);
        zval *z_iface  = zend_hash_str_find(Z_ARRVAL_P(zopts), "iface", sizeof("iface")-1);
        if (z_mode && Z_TYPE_P(z_mode) == IS_STRING) {
            mode = Z_STRVAL_P(z_mode);
            mode_len = Z_STRLEN_P(z_mode);
        }
        if (z_delay && Z_TYPE_P(z_delay) == IS_LONG) {
            delay_ms = Z_LVAL_P(z_delay);
            if (delay_ms < 0) delay_ms = 0;
        }
        if (z_iface && Z_TYPE_P(z_iface) == IS_STRING) {
            iface = Z_STRVAL_P(z_iface);
            iface_len = Z_STRLEN_P(z_iface);
        }
    }

    /* Fetch the config resource */
    quiche_config *cfg = quicpro_fetch_config(zcfg);
    if (!cfg) {
        quicpro_set_error("Invalid config resource");
        RETURN_FALSE;
    }

    /* Resolve DNS for host:port */
    char port_str[6];
    snprintf(port_str, sizeof(port_str), "%ld", port);
    struct addrinfo *ai_list = NULL;
    if (resolve_host(host, port_str, &ai_list) != 0) {
        quicpro_set_error("DNS lookup failed");
        RETURN_FALSE;
    }

    int fd = -1;
    struct addrinfo *ai;
    int family_used = AF_UNSPEC;
    int try_v6 = 0, try_v4 = 0;

    /* Determine family preferences */
    if (strncmp(mode, "ipv6", 4) == 0)      { try_v6 = 1; try_v4 = 0; }
    else if (strncmp(mode, "ipv4", 4) == 0) { try_v6 = 0; try_v4 = 1; }
    else                                    { try_v6 = 1; try_v4 = 1; }

    /* Try IPv6 first if allowed */
    if (try_v6) {
        for (ai = ai_list; ai; ai = ai->ai_next) {
            if (ai->ai_family != AF_INET6) continue;
            fd = udp_socket_nonblock(ai->ai_family);
            if (fd < 0) continue;
            if (iface && iface_len && socket_bind_iface(fd, iface) < 0) {
                close(fd); fd = -1; continue;
            }
            if (connect(fd, ai->ai_addr, ai->ai_addrlen) == 0) {
                family_used = AF_INET6;
                break;
            }
            close(fd); fd = -1;
        }
    }

    /* Happy Eyeballs delay, if needed (only if both families allowed) */
    if (fd < 0 && try_v6 && try_v4 && delay_ms > 0) {
        usleep(delay_ms * 1000); // delay in microseconds, can be 0
    }

    /* Try IPv4 if allowed and needed */
    if (fd < 0 && try_v4) {
        for (ai = ai_list; ai; ai = ai->ai_next) {
            if (ai->ai_family != AF_INET) continue;
            fd = udp_socket_nonblock(ai->ai_family);
            if (fd < 0) continue;
            if (iface && iface_len && socket_bind_iface(fd, iface) < 0) {
                close(fd); fd = -1; continue;
            }
            if (connect(fd, ai->ai_addr, ai->ai_addrlen) == 0) {
                family_used = AF_INET;
                break;
            }
            close(fd); fd = -1;
        }
    }

    /* If no socket could be created, cleanup and return FALSE */
    if (fd < 0) {
        freeaddrinfo(ai_list);
        quicpro_set_error("connect() failed");
        RETURN_FALSE;
    }

    /* Allocate and initialize session object */
    quicpro_session_t *s = ecalloc(1, sizeof(*s));
    s->sock = fd;
    s->cfg  = cfg;
    size_t copylen = (host_len < sizeof(s->host) - 1) ? host_len : sizeof(s->host) - 1;
    memcpy(s->host, host, copylen);
    s->host[copylen] = '\0';

    /* Generate source connection ID */
    uint8_t scid[16];
    RAND_bytes(scid, sizeof(scid));

    /* QUIC handshake */
    s->conn = quiche_connect(
        host,
        scid, sizeof(scid),
        NULL, 0,
        ai->ai_addr, ai->ai_addrlen,
        cfg
    );
    if (!s->conn) {
        freeaddrinfo(ai_list);
        efree(s);
        close(fd);
        quicpro_set_error("quiche_connect failed");
        RETURN_FALSE;
    }

    /* Attach HTTP/3 context */
    s->h3_cfg = quiche_h3_config_new();
    s->h3     = quiche_h3_conn_new_with_transport(s->conn, s->h3_cfg);

    freeaddrinfo(ai_list);
    RETURN_RES(zend_register_resource(s, le_quicpro));
}
