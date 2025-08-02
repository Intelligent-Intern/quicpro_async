/**
 * @file extension/src/server/http3.c
 * @brief Implementation of the HTTP/3 Server over QUIC.
 *
 * This file contains the C-level implementation for a native HTTP/3 server.
 * It leverages the quiche library to handle the QUIC transport protocol and
 * the HTTP/3 application protocol. The server uses a non-blocking I/O model
 * with epoll to manage a high number of concurrent connections and streams
 * efficiently.
 */

#include <php.h>
#include <zend_exceptions.h>
#include <zend_hash.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <time.h>

#include "quiche.h"
#include "server/http3.h"
#include "session/session.h" // Assuming this will define quicpro_session_t

// The core server object, holding its state.
typedef struct {
    int fd;
    int epoll_fd;
    HashTable *sessions_by_scid;
    quiche_config *quic_config;
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    bool is_listening;
} http3_server_t;

// Resource IDs to be defined in the main extension file during MINIT.
extern int le_quicpro_session;
extern zend_class_entry *quicpro_config_ce;

// Forward declaration for the internal session destructor.
void quicpro_session_dtor_internal(void *session);

// Generates a new cryptographically secure Connection ID.
static int generate_cid(uint8_t *cid, size_t len) {
    int urandom_fd = open("/dev/urandom", O_RDONLY);
    if (urandom_fd < 0) {
        return -1;
    }
    ssize_t result = read(urandom_fd, cid, len);
    close(urandom_fd);
    return (result == (ssize_t)len) ? 0 : -1;
}

// Helper to apply settings from the PHP Config object to the C quiche_config struct.
static void apply_config_to_quiche(quiche_config *quic_config, zval *config_obj)
{
    zval *options_zv = zend_read_property(quicpro_config_ce, Z_OBJ_P(config_obj), "options", sizeof("options") - 1, 1, NULL);
    if (Z_TYPE_P(options_zv) != IS_ARRAY) return;

    zval *val;
    if ((val = zend_hash_str_find(Z_ARRVAL_P(options_zv), "cert_file", sizeof("cert_file")-1)) && Z_TYPE_P(val) == IS_STRING) {
        quiche_config_load_cert_chain_from_pem_file(quic_config, Z_STRVAL_P(val));
    }
    if ((val = zend_hash_str_find(Z_ARRVAL_P(options_zv), "key_file", sizeof("key_file")-1)) && Z_TYPE_P(val) == IS_STRING) {
        quiche_config_load_priv_key_from_pem_file(quic_config, Z_STRVAL_P(val));
    }
    // Add other config mappings here...
}

PHP_FUNCTION(quicpro_http3_server_listen)
{
    char *host;
    size_t host_len;
    zend_long port;
    zval *config_resource;
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;

    ZEND_PARSE_PARAMETERS_START(5, 5)
        Z_PARAM_STRING(host, host_len)
        Z_PARAM_LONG(port)
        Z_PARAM_OBJECT_OF_CLASS(config_resource, quicpro_config_ce)
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();

    http3_server_t server;
    memset(&server, 0, sizeof(server));
    server.fci = fci;
    server.fcc = fcc;

    server.fd = socket(AF_INET6, SOCK_DGRAM, 0);
    if (server.fd < 0) {
        zend_throw_exception_ex(NULL, 0, "HTTP/3 server failed to create socket: %s", strerror(errno));
        RETURN_FALSE;
    }

    int reuse = 1;
    setsockopt(server.fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
    int off = 0;
    setsockopt(server.fd, IPPROTO_IPV6, IPV6_V6ONLY, &off, sizeof(off));

    struct sockaddr_in6 addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(port);
    inet_pton(AF_INET6, host, &addr.sin6_addr);

    if (bind(server.fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        zend_throw_exception_ex(NULL, 0, "HTTP/3 server failed to bind to %s:%ld: %s", host, port, strerror(errno));
        close(server.fd);
        RETURN_FALSE;
    }

    server.quic_config = quiche_config_new(QUICHE_PROTOCOL_VERSION);
    apply_config_to_quiche(server.quic_config, config_resource);
    
    ALLOC_HASHTABLE(server.sessions_by_scid);
    zend_hash_init(server.sessions_by_scid, 16, NULL, (dtor_func_t)quicpro_session_dtor_internal, 0);

    server.epoll_fd = epoll_create1(0);
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.ptr = &server;
    epoll_ctl(server.epoll_fd, EPOLL_CTL_ADD, server.fd, &event);

    #define MAX_EVENTS 64
    struct epoll_event events[MAX_EVENTS];
    uint8_t buffer[2048];
    server.is_listening = true;

    while (server.is_listening) {
        int n_events = epoll_wait(server.epoll_fd, events, MAX_EVENTS, 100);
        if (n_events < 0) {
            if (errno == EINTR) continue;
            break;
        }

        for (int i = 0; i < n_events; i++) {
            if (events[i].data.ptr == &server) {
                struct sockaddr_storage peer_addr;
                socklen_t peer_addr_len = sizeof(peer_addr);
                ssize_t read_len = recvfrom(server.fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&peer_addr, &peer_addr_len);

                if (read_len < 0) continue;

                uint8_t scid[QUICHE_MAX_CONN_ID_LEN], dcid[QUICHE_MAX_CONN_ID_LEN];
                size_t scid_len = sizeof(scid), dcid_len = sizeof(dcid);
                
                if (quiche_header_info(buffer, read_len, QUICHE_MAX_CONN_ID_LEN, dcid, &dcid_len, scid, &scid_len, NULL, NULL, NULL) < 0) {
                    continue;
                }

                quicpro_session_t *session = zend_hash_str_find_ptr(server.sessions_by_scid, (char*)dcid, dcid_len);

                if (session == NULL) {
                    if (generate_cid(scid, QUICHE_MAX_CONN_ID_LEN) < 0) continue;

                    quiche_conn *conn = quiche_accept(scid, QUICHE_MAX_CONN_ID_LEN, NULL, 0, (struct sockaddr *)&peer_addr, peer_addr_len, server.quic_config);
                    if (conn == NULL) continue;

                    session = ecalloc(1, sizeof(quicpro_session_t));
                    session->conn = conn;
                    memcpy(&session->peer_addr, &peer_addr, peer_addr_len);
                    session->peer_addr_len = peer_addr_len;
                    
                    zend_hash_str_add_ptr(server.sessions_by_scid, (char*)scid, QUICHE_MAX_CONN_ID_LEN, session);
                }

                quiche_conn_recv(session->conn, buffer, read_len, (struct sockaddr *)&peer_addr, peer_addr_len);
            }
        }

        zend_string *key;
        quicpro_session_t *session;
        ZEND_HASH_FOREACH_STR_KEY_PTR(server.sessions_by_scid, key, session) {
            quiche_conn_on_timeout(session->conn);

            uint8_t out[2048];
            ssize_t sent;
            do {
                sent = quiche_conn_send(session->conn, out, sizeof(out));
                if (sent == QUICHE_ERR_DONE) break;
                if (sent < 0) break;
                sendto(server.fd, out, sent, 0, (struct sockaddr *)&session->peer_addr, session->peer_addr_len);
            } while (1);

            if (quiche_conn_is_established(session->conn)) {
                quiche_stream_iter *readable = quiche_conn_readable(session->conn);
                uint64_t stream_id;
                while (quiche_stream_iter_next(readable, &stream_id)) {
                    zval args[2], retval;
                    session->resource = zend_register_resource(session, le_quicpro_session);
                    ZVAL_RES(&args[0], session->resource);
                    ZVAL_LONG(&args[1], stream_id);

                    server.fci.param_count = 2;
                    server.fci.params = args;
                    server.fci.retval = &retval;

                    if (zend_call_function(&server.fci, &server.fcc) == SUCCESS) {
                        zval_ptr_dtor(&retval);
                    }
                }
                quiche_stream_iter_free(readable);
            }

            if (quiche_conn_is_closed(session->conn)) {
                zend_hash_del(server.sessions_by_scid, key);
            }
        } ZEND_HASH_FOREACH_END();
    }
    
    close(server.epoll_fd);
    close(server.fd);
    zend_hash_destroy(server.sessions_by_scid);
    FREE_HASHTABLE(server.sessions_by_scid);
    quiche_config_free(server.quic_config);
    RETURN_TRUE;
}
