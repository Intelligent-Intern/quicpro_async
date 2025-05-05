/*  Open a QUIC connection using a pre‑built config
 *  -----------------------------------------------------------------------------
 *  Signature (PHP):
 *      resource quicpro_connect(string $host, int $port, resource $cfg [, string $iface])
 *
 *  • Accepts a `quicpro_config` Zend resource instead of ad‑hoc config
 *  • Sets SNI via `quiche_conn_set_tls_name()`
 *  • Supports IPv6 + Happy‑Eyeballs (RFC 8305) fallback to IPv4
 *  • Optional `$iface` enables SO_BINDTODEVICE – useful for worker‑per‑queue
 */
#include <stddef.h>
#include "php_quicpro.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <openssl/rand.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define HE_PROBE_DELAY_MS 250  /* Happy‑Eyeballs delay */

static int socket_bind_iface(int fd, const char *iface)
{
    /* returns 0 on success, -1 on error */
    return setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, iface, strlen(iface));
}

static int udp_socket_nonblock(int family)
{
    int fd = socket(family, SOCK_DGRAM | SOCK_NONBLOCK, 0);
    return fd;
}

static int resolve_host(const char *host, const char *port, struct addrinfo **res)
{
    struct addrinfo hints = {0};
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_family   = AF_UNSPEC;     /* IPv6 + v4 */
    return getaddrinfo(host, port, &hints, res);
}

PHP_FUNCTION(quicpro_connect)
{
    char *host;
    size_t host_len;
    zend_long port;
    zval *zcfg;
    char *iface = NULL;
    size_t iface_len = 0;

    ZEND_PARSE_PARAMETERS_START(3, 4)
        Z_PARAM_STRING(host, host_len)
        Z_PARAM_LONG(port)
        Z_PARAM_RESOURCE(zcfg)
        Z_PARAM_OPTIONAL
        Z_PARAM_STRING(iface, iface_len)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    quiche_config *cfg = quicpro_fetch_config(zcfg);
    if (!cfg) RETURN_FALSE;

    /* --- Happy‑Eyeballs resolve --- */
    char port_str[6];
    snprintf(port_str, sizeof(port_str), "%ld", port);
    struct addrinfo *ai_list;
    if (resolve_host(host, port_str, &ai_list) != 0) {
        quicpro_set_error("DNS lookup failed");
        RETURN_FALSE;
    }

    int fd = -1;
    struct addrinfo *ai;
    int family_used = AF_UNSPEC;

    /* First pass: IPv6 (per RFC) */
    for (ai = ai_list; ai; ai = ai->ai_next) {
        if (ai->ai_family != AF_INET6) continue;
        fd = udp_socket_nonblock(ai->ai_family);
        if (fd < 0) continue;
        if (iface_len && socket_bind_iface(fd, iface) < 0) { close(fd); fd = -1; continue; }
        if (connect(fd, ai->ai_addr, ai->ai_addrlen) == 0) { family_used = AF_INET6; break; }
        close(fd); fd = -1;
    }

    /* Short delay before v4 attempt – Happy‑Eyeballs */
    if (fd < 0) usleep(HE_PROBE_DELAY_MS * 1000);

    /* Second pass: IPv4 */
    if (fd < 0) {
        for (ai = ai_list; ai; ai = ai->ai_next) {
            if (ai->ai_family != AF_INET) continue;
            fd = udp_socket_nonblock(ai->ai_family);
            if (fd < 0) continue;
            if (iface_len && socket_bind_iface(fd, iface) < 0) { close(fd); fd = -1; continue; }
            if (connect(fd, ai->ai_addr, ai->ai_addrlen) == 0) { family_used = AF_INET; break; }
            close(fd); fd = -1;
        }
    }

    if (fd < 0) { freeaddrinfo(ai_list); quicpro_set_error("connect() failed"); RETURN_FALSE; }

    /* --- QUIC connection --- */
    quicpro_session_t *s = ecalloc(1, sizeof(*s));
    s->sock = fd;
    s->cfg  = cfg; /* reused – NOT freed until resource dtor */
    s->host = estrndup(host, host_len);

    uint8_t scid[16];
    RAND_bytes(scid, sizeof(scid));

    s->conn = quiche_connect(host,
                             scid, sizeof(scid),
                             NULL, 0,
                             ai->ai_addr, ai->ai_addrlen,
                             cfg);

    if (!s->conn) { freeaddrinfo(ai_list); efree(s->host); efree(s); close(fd); quicpro_set_error("quiche_connect failed"); RETURN_FALSE; }

    /* SNI / TLS name */
    quiche_conn_set_tls_name(s->conn, host, host_len);

    /* HTTP/3 context */
    s->h3_cfg = quiche_h3_config_new();
    s->h3     = quiche_h3_conn_new_with_transport(s->conn, s->h3_cfg);

    freeaddrinfo(ai_list);

    ZEND_REGISTER_RESOURCE(return_value, s, le_quicpro);
}
