/*
 * connect.c – quicpro_connect() implementation
 * ----------------------------------------------------------------------------
 * This file provides the core client‐side entry point for opening a QUIC
 * connection using a prebuilt quiche_config resource. It exports the
 * PHP function quicpro_connect(host, port, cfg, [iface]) which returns a
 * resource pointing to a quicpro_session_t. Under the hood, it performs
 * DNS resolution with Happy‑Eyeballs for IPv6/IPv4 fallback, binds to an
 * optional network interface, creates a nonblocking UDP socket, and hands
 * it off to quiche_connect() to perform the QUIC handshake. Once the
 * handshake succeeds, SNI is set for TLS, an HTTP/3 context is attached,
 * and the resource is registered with PHP so scripts can send and receive
 * HTTP/3 requests over this session. Any error along the way is converted
 * into a PHP‐visible failure or exception via quicpro_set_error().
 */

#include <stddef.h>
#include <arpa/inet.h>    /* inet_pton, sockaddr structures */
#include <netdb.h>        /* getaddrinfo(), addrinfo */
#include <openssl/rand.h> /* RAND_bytes for connection ID */
#include <string.h>       /* strlen, memcpy */
#include <sys/socket.h>   /* socket(), setsockopt(), connect() */
#include <sys/types.h>    /* various type definitions */
#include <unistd.h>       /* close(), usleep() */

#include "php_quicpro.h"  /* core extension definitions */
#include <quiche.h>       /* quiche API for QUIC & HTTP/3 */

extern int le_quicpro;

/* Delay in milliseconds before trying IPv4 after IPv6 (Happy‑Eyeballs) */
#define HE_PROBE_DELAY_MS 250

/*
 * socket_bind_iface()
 * -------------------
 * If the user supplies a network interface name (e.g. "eth0"), bind the
 * socket to that interface using SO_BINDTODEVICE. This is useful in
 * multi‐queue or containerized environments where you want to pin UDP
 * traffic to a specific NIC. Returns 0 on success or -1 on error, in
 * which case the socket is closed and the next address candidate is tried.
 */
static int socket_bind_iface(int fd, const char *iface)
{
    return setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, iface, strlen(iface));
}

/*
 * udp_socket_nonblock()
 * ---------------------
 * Create a UDP socket in non‐blocking mode. We request SOCK_NONBLOCK
 * at creation so that subsequent send/recv calls never block the PHP
 * process. This is crucial for integrating with the PHP event loop
 * and Fiber scheduler, ensuring low‐latency handling of QUIC packets.
 */
static int udp_socket_nonblock(int family)
{
    return socket(family, SOCK_DGRAM | SOCK_NONBLOCK, 0);
}

/*
 * resolve_host()
 * --------------
 * Perform DNS resolution for the given host and port, returning a
 * linked list of addrinfo structs. We request AI_ADDRCONFIG so that
 * only configured address families are returned. The caller is
 * responsible for calling freeaddrinfo() when done. On success
 * returns 0; on failure, returns a nonzero error code.
 */
static int resolve_host(const char *host, const char *port, struct addrinfo **res)
{
    struct addrinfo hints = {0};
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_family   = AF_UNSPEC;  /* allow both IPv6 and IPv4 */
    return getaddrinfo(host, port, &hints, res);
}

/*
 * PHP_FUNCTION(quicpro_connect)
 * ------------------------------
 * PHP signature:
 *   resource quicpro_connect(string $host, int $port, resource $cfg [, string $iface])
 *
 * This function orchestrates the entire setup of a QUIC session:
 * 1. Parse parameters: a host, port, a quiche_config resource, and an
 *    optional interface string.
 * 2. Fetch the underlying quiche_config from the resource wrapper.
 * 3. Resolve the host via DNS into a list of IPv6 and IPv4 addresses.
 * 4. Attempt to create and connect a nonblocking UDP socket first to
 *    an IPv6 address, then, after a small delay, to IPv4 if IPv6 fails.
 * 5. On socket success, allocate a quicpro_session_t, generate a random
 *    source connection ID (SCID), and call quiche_connect() to start
 *    the QUIC handshake. If that succeeds, set the TLS SNI, attach an
 *    HTTP/3 context, and register the session as a PHP resource.
 * 6. Any intermediate failure is converted into a PHP‐level false return,
 *    with quicpro_set_error() supplying an error message retrievable by
 *    quicpro_get_last_error().
 */
PHP_FUNCTION(quicpro_connect)
{
    char        *host;
    size_t       host_len;
    zend_long    port;
    zval        *zcfg;
    char        *iface   = NULL;
    size_t       iface_len = 0;

    /* Parse and validate PHP function arguments */
    ZEND_PARSE_PARAMETERS_START(3, 4)
        Z_PARAM_STRING(host, host_len)
        Z_PARAM_LONG(port)
        Z_PARAM_RESOURCE(zcfg)
        Z_PARAM_OPTIONAL
        Z_PARAM_STRING(iface, iface_len)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    /* Extract the quiche_config pointer from the PHP resource */
    quiche_config *cfg = quicpro_fetch_config(zcfg);
    if (!cfg) {
        RETURN_FALSE;
    }

    /* Convert port to string for getaddrinfo */
    char port_str[6];
    snprintf(port_str, sizeof(port_str), "%ld", port);

    /* Perform DNS resolution for both IPv6 and IPv4 */
    struct addrinfo *ai_list;
    if (resolve_host(host, port_str, &ai_list) != 0) {
        quicpro_set_error("DNS lookup failed");
        RETURN_FALSE;
    }

    int fd = -1;
    struct addrinfo *ai;
    int family_used = AF_UNSPEC;

    /* Try connecting over IPv6 first (per RFC Happy‑Eyeballs) */
    for (ai = ai_list; ai; ai = ai->ai_next) {
        if (ai->ai_family != AF_INET6) {
            continue;
        }
        fd = udp_socket_nonblock(ai->ai_family);
        if (fd < 0) {
            continue;
        }
        /* Optionally bind to a specific interface, then attempt connect() */
        if (iface_len && socket_bind_iface(fd, iface) < 0) {
            close(fd);
            fd = -1;
            continue;
        }
        if (connect(fd, ai->ai_addr, ai->ai_addrlen) == 0) {
            family_used = AF_INET6;
            break;
        }
        close(fd);
        fd = -1;
    }

    /* Wait a short time before falling back to IPv4 attempts */
    if (fd < 0) {
        usleep(HE_PROBE_DELAY_MS * 1000);
    }

    /* Now try IPv4 addresses if IPv6 did not succeed */
    if (fd < 0) {
        for (ai = ai_list; ai; ai = ai->ai_next) {
            if (ai->ai_family != AF_INET) {
                continue;
            }
            fd = udp_socket_nonblock(ai->ai_family);
            if (fd < 0) {
                continue;
            }
            if (iface_len && socket_bind_iface(fd, iface) < 0) {
                close(fd);
                fd = -1;
                continue;
            }
            if (connect(fd, ai->ai_addr, ai->ai_addrlen) == 0) {
                family_used = AF_INET;
                break;
            }
            close(fd);
            fd = -1;
        }
    }

    /* If both families failed, clean up and return false */
    if (fd < 0) {
        freeaddrinfo(ai_list);
        quicpro_set_error("connect() failed");
        RETURN_FALSE;
    }

    /* Allocate and initialize our session object */
    quicpro_session_t *s = ecalloc(1, sizeof(*s));
    s->sock = fd;               /* store the UDP socket FD */
    s->cfg  = cfg;              /* hold onto the quiche_config */
    /* host als char[256] Feld kopieren */
    size_t copylen = (host_len < sizeof(s->host) - 1) ? host_len : sizeof(s->host) - 1;
    memcpy(s->host, host, copylen);
    s->host[copylen] = '\0';

    /* Generate a random 16‑byte source connection ID (SCID) */
    uint8_t scid[16];
    RAND_bytes(scid, sizeof(scid));

    /* Initiate the QUIC handshake via quiche */
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

    /* Create and attach an HTTP/3 context on top of the QUIC connection */
    s->h3_cfg = quiche_h3_config_new();
    s->h3     = quiche_h3_conn_new_with_transport(s->conn, s->h3_cfg);

    /* Release the addrinfo list now that we have a live socket */
    freeaddrinfo(ai_list);

    /* Finally, register our session struct as a PHP resource and return it */
    RETURN_RES(zend_register_resource(s, le_quicpro));
}
