/**
 * @file extension/src/server/http1.c
 * @brief Implementation of the HTTP/1.1 Server.
 *
 * This file contains the C-level implementation for a standalone, high-performance
 * HTTP/1.1 server over TCP. It utilizes a non-blocking I/O model with epoll
 * to handle a large number of concurrent connections efficiently within a
 * single thread, invoking a user-provided PHP handler for each request.
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
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "server/http1.h"

#define READ_BUFFER_SIZE 8192

typedef struct http1_server_s http1_server_t;

// Represents the state of a single client connection
typedef struct {
    int fd;
    SSL *ssl;
    char read_buffer[READ_BUFFER_SIZE];
    size_t read_buffer_len;
    char *write_buffer;
    size_t write_buffer_len;
    size_t write_buffer_sent;
    enum {
        STATE_HANDSHAKING,
        STATE_READING,
        STATE_WRITING,
        STATE_CLOSING
    } state;
    http1_server_t *server;
} http1_client_connection_t;

// Represents the state of the main HTTP/1.1 server
struct http1_server_s {
    int listen_fd;
    int epoll_fd;
    SSL_CTX *ssl_ctx; // Default SSL context
    HashTable *vhost_contexts; // Maps hostname -> SSL_CTX*
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    bool is_listening;
};

// Forward declarations
static void handle_client_event(http1_client_connection_t *conn, uint32_t events);
static void close_client_connection(http1_client_connection_t *conn);
extern zend_class_entry *quicpro_config_ce;

static int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// SNI callback to select the correct SSL_CTX for a vhost
static int sni_callback(SSL *ssl, int *al, void *arg) {
    http1_server_t *server = (http1_server_t *)arg;
    const char *servername = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);

    if (servername) {
        SSL_CTX *vhost_ctx = zend_hash_str_find_ptr(server->vhost_contexts, servername, strlen(servername));
        if (vhost_ctx) {
            SSL_set_SSL_CTX(ssl, vhost_ctx);
            return SSL_TLSEXT_ERR_OK;
        }
    }
    // Fallback to the default context if no match is found
    return SSL_TLSEXT_ERR_OK;
}

PHP_FUNCTION(quicpro_http1_server_listen)
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

    http1_server_t server;
    memset(&server, 0, sizeof(server));
    server.fci = fci;
    server.fcc = fcc;

    // A full implementation would parse vhosts from the config and create an SSL_CTX for each.
    // For now, we create a single default context.
    const char *cert_file = "path/to/default-cert.pem";
    const char *key_file = "path/to/default-key.pem";

    server.ssl_ctx = SSL_CTX_new(TLS_server_method());
    SSL_CTX_set_options(server.ssl_ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1);
    SSL_CTX_set_tlsext_servername_callback(server.ssl_ctx, sni_callback);
    SSL_CTX_set_tlsext_servername_arg(server.ssl_ctx, &server);
    SSL_CTX_use_certificate_file(server.ssl_ctx, cert_file, SSL_FILETYPE_PEM);
    SSL_CTX_use_private_key_file(server.ssl_ctx, key_file, SSL_FILETYPE_PEM);

    server.listen_fd = socket(AF_INET6, SOCK_STREAM, 0);
    // ... (full socket, setsockopt, bind, listen, set_nonblocking calls) ...

    server.epoll_fd = epoll_create1(0);
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    event.data.ptr = &server;
    epoll_ctl(server.epoll_fd, EPOLL_CTL_ADD, server.listen_fd, &event);

    #define MAX_EVENTS 128
    struct epoll_event events[MAX_EVENTS];
    server.is_listening = true;

    while (server.is_listening) {
        int n_events = epoll_wait(server.epoll_fd, events, MAX_EVENTS, -1);
        for (int i = 0; i < n_events; i++) {
            if (events[i].data.ptr == &server) {
                while (1) {
                    int client_fd = accept(server.listen_fd, NULL, NULL);
                    if (client_fd < 0) break;
                    set_nonblocking(client_fd);

                    http1_client_connection_t *conn = ecalloc(1, sizeof(http1_client_connection_t));
                    conn->fd = client_fd;
                    conn->server = &server;
                    conn->state = STATE_HANDSHAKING;
                    conn->ssl = SSL_new(server.ssl_ctx);
                    SSL_set_fd(conn->ssl, client_fd);

                    event.events = EPOLLIN | EPOLLOUT | EPOLLET;
                    event.data.ptr = conn;
                    epoll_ctl(server.epoll_fd, EPOLL_CTL_ADD, client_fd, &event);
                }
            } else {
                handle_client_event((http1_client_connection_t *)events[i].data.ptr, events[i].events);
            }
        }
    }

    close(server.listen_fd);
    close(server.epoll_fd);
    SSL_CTX_free(server.ssl_ctx);
    // zend_hash_destroy(server.vhost_contexts);
    // FREE_HASHTABLE(server.vhost_contexts);
    RETURN_TRUE;
}

static void close_client_connection(http1_client_connection_t *conn) {
    epoll_ctl(conn->server->epoll_fd, EPOLL_CTL_DEL, conn->fd, NULL);
    SSL_shutdown(conn->ssl);
    SSL_free(conn->ssl);
    close(conn->fd);
    if (conn->write_buffer) efree(conn->write_buffer);
    efree(conn);
}

static void handle_client_event(http1_client_connection_t *conn, uint32_t events) {
    if (conn->state == STATE_HANDSHAKING) {
        int ret = SSL_accept(conn->ssl);
        if (ret == 1) {
            conn->state = STATE_READING;
        } else if (SSL_get_error(conn->ssl, ret) != SSL_ERROR_WANT_READ && SSL_get_error(conn->ssl, ret) != SSL_ERROR_WANT_WRITE) {
            close_client_connection(conn);
            return;
        }
    }

    if ((events & EPOLLIN) && conn->state == STATE_READING) {
        ssize_t count = SSL_read(conn->ssl, conn->read_buffer + conn->read_buffer_len, READ_BUFFER_SIZE - conn->read_buffer_len);
        if (count <= 0) {
            close_client_connection(conn);
            return;
        }
        conn->read_buffer_len += count;

        char *header_end = strstr(conn->read_buffer, "\r\n\r\n");
        if (header_end) {
            zval request_zv, retval;
            array_init(&request_zv);
            // Full request parsing logic here...
            add_assoc_string(&request_zv, "method", "GET");
            add_assoc_string(&request_zv, "uri", "/");

            conn->server->fci.param_count = 1;
            conn->server->fci.params = &request_zv;
            conn->server->fci.retval = &retval;

            if (zend_call_function(&conn->server->fci, &conn->server->fcc) == SUCCESS && Z_TYPE(retval) == IS_ARRAY) {
                zval *status_zv = zend_hash_str_find(Z_ARRVAL(retval), "status", sizeof("status")-1);
                zval *body_zv = zend_hash_str_find(Z_ARRVAL(retval), "body", sizeof("body")-1);

                long status = status_zv ? zval_get_long(status_zv) : 200;
                char *body = body_zv ? Z_STRVAL_P(body_zv) : "";
                size_t body_len = body_zv ? Z_STRLEN_P(body_zv) : 0;

                conn->write_buffer_len = spprintf(&conn->write_buffer, 0,
                    "HTTP/1.1 %ld OK\r\nContent-Length: %zu\r\nConnection: close\r\n\r\n%s",
                    status, body_len, body);

                conn->state = STATE_WRITING;
                conn->write_buffer_sent = 0;
            }
            zval_ptr_dtor(&request_zv);
            zval_ptr_dtor(&retval);
        }
    }

    if ((events & EPOLLOUT) && conn->state == STATE_WRITING) {
        ssize_t sent = SSL_write(conn->ssl, conn->write_buffer + conn->write_buffer_sent, conn->write_buffer_len - conn->write_buffer_sent);
        if (sent > 0) {
            conn->write_buffer_sent += sent;
        } else if (SSL_get_error(conn->ssl, sent) != SSL_ERROR_WANT_WRITE) {
            close_client_connection(conn);
            return;
        }

        if (conn->write_buffer_sent >= conn->write_buffer_len) {
            // A full implementation would check for "Connection: keep-alive"
            // and reset the state to STATE_READING instead of closing.
            close_client_connection(conn);
        }
    }
}
