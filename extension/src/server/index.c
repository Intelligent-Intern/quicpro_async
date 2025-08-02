/**
 * @file extension/src/server/index.c
 * @brief Implementation of the core, protocol-agnostic server functionalities.
 *
 * This file contains the C-level implementation for the functions declared in
 * `server.h`. It handles the low-level system calls required to create, bind,
 * and manage server sockets, and runs the main event loop to process QUIC
 * connections. It serves as the foundation upon which all higher-level server
 * logic and protocol handlers are built.
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
#include "server/index.h"
#include "session/session.h"

// The core server object, holding its state.
typedef struct {
    int fd;
    int epoll_fd;
    HashTable *sessions_by_scid;
    quiche_config *quic_config;
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    bool is_listening;
} quicpro_server_t;

// Resource IDs to be defined in the main extension file during MINIT.
extern int le_quicpro_server;
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

// Helper function to apply settings from the PHP Config object to the C quiche_config struct.
static void apply_php_config_to_quiche(quiche_config *quic_config, zval *config_obj)
{
    zval *options_zv, *value;
    zend_string *key;

    options_zv = zend_read_property(quicpro_config_ce, Z_OBJ_P(config_obj), "options", sizeof("options") - 1, 1, NULL);
    if (Z_TYPE_P(options_zv) != IS_ARRAY) {
        return; // No options array found or it's not an array.
    }

    ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(options_zv), key, value) {
        if (!key) continue;

        if (zend_string_equals_literal(key, "application_protos")) {
            if (Z_TYPE_P(value) == IS_STRING) {
                quiche_config_set_application_protos(quic_config, (uint8_t *)Z_STRVAL_P(value), Z_STRLEN_P(value));
            }
        } else if (zend_string_equals_literal(key, "max_idle_timeout")) {
            if (Z_TYPE_P(value) == IS_LONG) {
                quiche_config_set_max_idle_timeout(quic_config, Z_LVAL_P(value));
            }
        } else if (zend_string_equals_literal(key, "max_recv_udp_payload_size")) {
            if (Z_TYPE_P(value) == IS_LONG) {
                quiche_config_set_max_recv_udp_payload_size(quic_config, Z_LVAL_P(value));
            }
        } else if (zend_string_equals_literal(key, "max_send_udp_payload_size")) {
            if (Z_TYPE_P(value) == IS_LONG) {
                quiche_config_set_max_send_udp_payload_size(quic_config, Z_LVAL_P(value));
            }
        } else if (zend_string_equals_literal(key, "initial_max_data")) {
            if (Z_TYPE_P(value) == IS_LONG) {
                quiche_config_set_initial_max_data(quic_config, Z_LVAL_P(value));
            }
        } else if (zend_string_equals_literal(key, "initial_max_stream_data_bidi_local")) {
            if (Z_TYPE_P(value) == IS_LONG) {
                quiche_config_set_initial_max_stream_data_bidi_local(quic_config, Z_LVAL_P(value));
            }
        } else if (zend_string_equals_literal(key, "initial_max_stream_data_bidi_remote")) {
            if (Z_TYPE_P(value) == IS_LONG) {
                quiche_config_set_initial_max_stream_data_bidi_remote(quic_config, Z_LVAL_P(value));
            }
        } else if (zend_string_equals_literal(key, "initial_max_stream_data_uni")) {
            if (Z_TYPE_P(value) == IS_LONG) {
                quiche_config_set_initial_max_stream_data_uni(quic_config, Z_LVAL_P(value));
            }
        } else if (zend_string_equals_literal(key, "initial_max_streams_bidi")) {
            if (Z_TYPE_P(value) == IS_LONG) {
                quiche_config_set_initial_max_streams_bidi(quic_config, Z_LVAL_P(value));
            }
        } else if (zend_string_equals_literal(key, "initial_max_streams_uni")) {
            if (Z_TYPE_P(value) == IS_LONG) {
                quiche_config_set_initial_max_streams_uni(quic_config, Z_LVAL_P(value));
            }
        } else if (zend_string_equals_literal(key, "cert_file")) {
            if (Z_TYPE_P(value) == IS_STRING) {
                quiche_config_load_cert_chain_from_pem_file(quic_config, Z_STRVAL_P(value));
            }
        } else if (zend_string_equals_literal(key, "key_file")) {
            if (Z_TYPE_P(value) == IS_STRING) {
                quiche_config_load_priv_key_from_pem_file(quic_config, Z_STRVAL_P(value));
            }
        }
    } ZEND_HASH_FOREACH_END();
}


PHP_FUNCTION(quicpro_server_create)
{
    char *host;
    size_t host_len;
    zend_long port;
    zval *config_resource;

    ZEND_PARSE_PARAMETERS_START(3, 3)
        Z_PARAM_STRING(host, host_len)
        Z_PARAM_LONG(port)
        Z_PARAM_OBJECT_OF_CLASS(config_resource, quicpro_config_ce)
    ZEND_PARSE_PARAMETERS_END();

    quicpro_server_t *server = ecalloc(1, sizeof(quicpro_server_t));
    server->fd = -1;
    server->epoll_fd = -1;

    server->fd = socket(AF_INET6, SOCK_DGRAM, 0);
    if (server->fd < 0) {
        zend_throw_exception_ex(NULL, 0, "Failed to create server socket: %s", strerror(errno));
        efree(server);
        RETURN_NULL();
    }

    int reuse = 1;
    if (setsockopt(server->fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0) {
        zend_throw_exception_ex(NULL, 0, "Failed to set SO_REUSEPORT: %s", strerror(errno));
        close(server->fd);
        efree(server);
        RETURN_NULL();
    }

    int off = 0;
    if (setsockopt(server->fd, IPPROTO_IPV6, IPV6_V6ONLY, &off, sizeof(off)) < 0) {
        zend_throw_exception_ex(NULL, 0, "Failed to disable IPV6_V6ONLY: %s", strerror(errno));
        close(server->fd);
        efree(server);
        RETURN_NULL();
    }

    struct sockaddr_in6 addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(port);

    if (inet_pton(AF_INET6, host, &addr.sin6_addr) != 1) {
        struct in_addr ipv4_addr;
        if (inet_pton(AF_INET, host, &ipv4_addr) == 1) {
            memcpy(&addr.sin6_addr.s6_addr[12], &ipv4_addr.s_addr, 4);
            addr.sin6_addr.s6_addr[10] = 0xff;
            addr.sin6_addr.s6_addr[11] = 0xff;
        } else {
            zend_throw_exception_ex(NULL, 0, "Invalid host address provided: %s", host);
            close(server->fd);
            efree(server);
            RETURN_NULL();
        }
    }

    if (bind(server->fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        zend_throw_exception_ex(NULL, 0, "Failed to bind server to %s:%ld: %s", host, port, strerror(errno));
        close(server->fd);
        efree(server);
        RETURN_NULL();
    }

    server->quic_config = quiche_config_new(QUICHE_PROTOCOL_VERSION);
    if (server->quic_config == NULL) {
        zend_throw_exception_ex(NULL, 0, "Failed to create quiche config");
        close(server->fd);
        efree(server);
        RETURN_NULL();
    }

    apply_php_config_to_quiche(server->quic_config, config_resource);

    ALLOC_HASHTABLE(server->sessions_by_scid);
    zend_hash_init(server->sessions_by_scid, 16, NULL, (dtor_func_t)quicpro_session_dtor_internal, 0);

    RETURN_RES(zend_register_resource(server, le_quicpro_server));
}

PHP_FUNCTION(quicpro_server_listen)
{
    zval *server_resource;
    quicpro_server_t *server;
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_RESOURCE(server_resource)
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();

    server = (quicpro_server_t *)zend_fetch_resource(Z_RES_P(server_resource), "quicpro_server", le_quicpro_server);
    server->fci = fci;
    server->fcc = fcc;

    server->epoll_fd = epoll_create1(0);
    if (server->epoll_fd == -1) {
        zend_throw_exception_ex(NULL, 0, "Failed to create epoll instance: %s", strerror(errno));
        RETURN_FALSE;
    }

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.ptr = server;
    if (epoll_ctl(server->epoll_fd, EPOLL_CTL_ADD, server->fd, &event) == -1) {
        zend_throw_exception_ex(NULL, 0, "Failed to add server socket to epoll: %s", strerror(errno));
        close(server->epoll_fd);
        RETURN_FALSE;
    }

    #define MAX_EVENTS 64
    struct epoll_event events[MAX_EVENTS];
    uint8_t buffer[2048];
    server->is_listening = true;

    while (server->is_listening) {
        int n_events = epoll_wait(server->epoll_fd, events, MAX_EVENTS, 100);
        if (n_events == -1) {
            if (errno == EINTR) continue;
            zend_throw_exception_ex(NULL, 0, "epoll_wait failed: %s", strerror(errno));
            break;
        }

        for (int i = 0; i < n_events; i++) {
            if (events[i].data.ptr == server) {
                struct sockaddr_storage peer_addr;
                socklen_t peer_addr_len = sizeof(peer_addr);
                ssize_t read_len = recvfrom(server->fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&peer_addr, &peer_addr_len);

                if (read_len < 0) continue;

                uint8_t scid[QUICHE_MAX_CONN_ID_LEN];
                uint8_t dcid[QUICHE_MAX_CONN_ID_LEN];
                size_t scid_len = sizeof(scid);
                size_t dcid_len = sizeof(dcid);

                if (quiche_header_info(buffer, read_len, QUICHE_MAX_CONN_ID_LEN, dcid, &dcid_len, scid, &scid_len, NULL, NULL, NULL) < 0) {
                    continue;
                }

                quicpro_session_t *session = zend_hash_str_find_ptr(server->sessions_by_scid, (char*)dcid, dcid_len);

                if (session == NULL) {
                    if (generate_cid(scid, QUICHE_MAX_CONN_ID_LEN) < 0) {
                        continue;
                    }

                    quiche_conn *conn = quiche_accept(scid, QUICHE_MAX_CONN_ID_LEN, NULL, 0, (struct sockaddr *)&peer_addr, peer_addr_len, server->quic_config);
                    if (conn == NULL) continue;

                    session = ecalloc(1, sizeof(quicpro_session_t));
                    session->conn = conn;
                    memcpy(&session->peer_addr, &peer_addr, peer_addr_len);
                    session->peer_addr_len = peer_addr_len;

                    zend_hash_str_add_ptr(server->sessions_by_scid, (char*)scid, QUICHE_MAX_CONN_ID_LEN, session);
                }

                quiche_conn_recv(session->conn, buffer, read_len, (struct sockaddr *)&peer_addr, peer_addr_len);
            }
        }

        zend_string *key;
        quicpro_session_t *session;
        ZEND_HASH_FOREACH_STR_KEY_PTR(server->sessions_by_scid, key, session) {
            quiche_conn_on_timeout(session->conn);

            ssize_t sent;
            uint8_t out[2048];
            do {
                sent = quiche_conn_send(session->conn, out, sizeof(out));
                if (sent == QUICHE_ERR_DONE) break;
                if (sent < 0) {
                    break;
                }
                sendto(server->fd, out, sent, 0, (struct sockaddr *)&session->peer_addr, session->peer_addr_len);
            } while (1);

            if (quiche_conn_is_established(session->conn)) {
                quiche_stream_iter *readable = quiche_conn_readable(session->conn);
                uint64_t stream_id;
                while (quiche_stream_iter_next(readable, &stream_id)) {
                    zval args[2];
                    zval retval;

                    session->resource = zend_register_resource(session, le_quicpro_session);
                    ZVAL_RES(&args[0], session->resource);
                    ZVAL_LONG(&args[1], stream_id);

                    server->fci.param_count = 2;
                    server->fci.params = args;
                    server->fci.retval = &retval;

                    if (zend_call_function(&server->fci, &server->fcc) == SUCCESS) {
                        zval_ptr_dtor(&retval);
                    }
                }
                quiche_stream_iter_free(readable);
            }

            if (quiche_conn_is_closed(session->conn)) {
                zend_hash_del(server->sessions_by_scid, key);
            }
        } ZEND_HASH_FOREACH_END();
    }

    close(server->epoll_fd);
    RETURN_TRUE;
}

PHP_FUNCTION(quicpro_server_close)
{
    zval *server_resource;
    quicpro_server_t *server;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_RESOURCE(server_resource)
    ZEND_PARSE_PARAMETERS_END();

    server = (quicpro_server_t *)zend_fetch_resource(Z_RES_P(server_resource), "quicpro_server", le_quicpro_server);
    server->is_listening = false;

    if (server->sessions_by_scid) {
        zend_hash_destroy(server->sessions_by_scid);
        FREE_HASHTABLE(server->sessions_by_scid);
        server->sessions_by_scid = NULL;
    }

    if (server->fd >= 0) {
        close(server->fd);
        server->fd = -1;
    }

    if (server->quic_config) {
        quiche_config_free(server->quic_config);
        server->quic_config = NULL;
    }

    zend_list_close(Z_RES_P(server_resource));
    RETURN_TRUE;
}
