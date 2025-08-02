/**
 * @file extension/src/server/http2.c
 * @brief Implementation of the HTTP/2 Server.
 *
 * This file contains the C-level implementation for a high-performance HTTP/2
 * server over TCP/TLS. It leverages a non-blocking I/O model with epoll and
 * the nghttp2 library to handle the protocol's complexities, such as stream
 * multiplexing, flow control, and header compression.
 */

#include <php.h>
#include <zend_exceptions.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <nghttp2/nghttp2.h>

#include "server/http2.h"

#define READ_BUFFER_SIZE 16384

typedef struct http2_server_s http2_server_t;
typedef struct http2_session_s http2_session_t;

// Represents the data source for a response body
typedef struct {
    char *data;
    size_t len;
    size_t offset;
} response_body_data_source;

// Represents a single HTTP/2 stream
typedef struct {
    int32_t stream_id;
    zval request_headers;
    char *request_body;
    size_t request_body_len;
    http2_session_t *session;
    response_body_data_source response_body;
} http2_stream_t;

// Represents a single client connection (session)
struct http2_session_s {
    int fd;
    SSL *ssl;
    nghttp2_session *ngh2_session;
    http2_server_t *server;
};

// Represents the main HTTP/2 server
struct http2_server_s {
    int listen_fd;
    int epoll_fd;
    SSL_CTX *ssl_ctx;
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    bool is_listening;
};

// Forward declarations
static ssize_t send_callback(nghttp2_session *session, const uint8_t *data, size_t length, int flags, void *user_data);
static int on_header_callback(nghttp2_session *session, const nghttp2_frame *frame, const uint8_t *name, size_t namelen, const uint8_t *value, size_t valuelen, uint8_t flags, void *user_data);
static int on_begin_headers_callback(nghttp2_session *session, const nghttp2_frame *frame, void *user_data);
static int on_data_chunk_recv_callback(nghttp2_session *session, uint8_t flags, int32_t stream_id, const uint8_t *data, size_t len, void *user_data);
static int on_stream_close_callback(nghttp2_session *session, int32_t stream_id, uint32_t error_code, void *user_data);
static int on_frame_recv_callback(nghttp2_session *session, const nghttp2_frame *frame, void *user_data);
static ssize_t response_read_callback(nghttp2_session *session, int32_t stream_id, uint8_t *buf, size_t length, uint32_t *data_flags, nghttp2_data_source *source, void *user_data);
static void handle_client_event(http2_session_t *session_data, uint32_t events);
static void close_http2_session(http2_session_t *session);

extern zend_class_entry *quicpro_config_ce;

static int set_nonblocking(int fd) {
    int flags;
    if ((flags = fcntl(fd, F_GETFL, 0)) == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static int alpn_select_proto_cb(SSL *ssl, const unsigned char **out, unsigned char *outlen,
                                const unsigned char *in, unsigned int inlen, void *arg) {
    if (nghttp2_select_next_protocol((unsigned char **)out, outlen, in, inlen) <= 0) {
        return SSL_TLSEXT_ERR_NOACK;
    }
    return SSL_TLSEXT_ERR_OK;
}

PHP_FUNCTION(quicpro_http2_server_listen)
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

    zval *options_zv = zend_read_property(quicpro_config_ce, Z_OBJ_P(config_resource), "options", sizeof("options") - 1, 1, NULL);
    zval *val;
    char *cert_file = NULL, *key_file = NULL;
    if (Z_TYPE_P(options_zv) == IS_ARRAY) {
        if ((val = zend_hash_str_find(Z_ARRVAL_P(options_zv), "cert_file", sizeof("cert_file")-1)) && Z_TYPE_P(val) == IS_STRING) cert_file = Z_STRVAL_P(val);
        if ((val = zend_hash_str_find(Z_ARRVAL_P(options_zv), "key_file", sizeof("key_file")-1)) && Z_TYPE_P(val) == IS_STRING) key_file = Z_STRVAL_P(val);
    }
    if (!cert_file || !key_file) {
        zend_throw_exception(NULL, "HTTP/2 server requires 'cert_file' and 'key_file' in configuration.", 0);
        RETURN_FALSE;
    }

    http2_server_t server;
    memset(&server, 0, sizeof(server));
    server.fci = fci;
    server.fcc = fcc;

    server.ssl_ctx = SSL_CTX_new(TLS_server_method());
    SSL_CTX_set_alpn_select_cb(server.ssl_ctx, alpn_select_proto_cb, NULL);
    if (SSL_CTX_use_certificate_file(server.ssl_ctx, cert_file, SSL_FILETYPE_PEM) <= 0 ||
        SSL_CTX_use_private_key_file(server.ssl_ctx, key_file, SSL_FILETYPE_PEM) <= 0) {
        SSL_CTX_free(server.ssl_ctx);
        zend_throw_exception(NULL, "Failed to load TLS certificate/key.", 0);
        RETURN_FALSE;
    }

    server.listen_fd = socket(AF_INET6, SOCK_STREAM, 0);
    int reuse = 1;
    setsockopt(server.listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    int off = 0;
    setsockopt(server.listen_fd, IPPROTO_IPV6, IPV6_V6ONLY, &off, sizeof(off));

    struct sockaddr_in6 addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(port);
    inet_pton(AF_INET6, host, &addr.sin6_addr);

    if (bind(server.listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0 || set_nonblocking(server.listen_fd) < 0 || listen(server.listen_fd, SOMAXCONN) < 0) {
        zend_throw_exception_ex(NULL, 0, "HTTP/2 server failed to bind/listen: %s", strerror(errno));
        close(server.listen_fd);
        SSL_CTX_free(server.ssl_ctx);
        RETURN_FALSE;
    }

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
        for (int i = 0; i < n_events; ++i) {
            if (events[i].data.ptr == &server) {
                while (1) {
                    int client_fd = accept(server.listen_fd, NULL, NULL);
                    if (client_fd < 0) break;
                    set_nonblocking(client_fd);
                    
                    http2_session_t *session_data = ecalloc(1, sizeof(http2_session_t));
                    session_data->fd = client_fd;
                    session_data->server = &server;
                    session_data->ssl = SSL_new(server.ssl_ctx);
                    SSL_set_fd(session_data->ssl, client_fd);
                    
                    struct epoll_event ev;
                    ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
                    ev.data.ptr = session_data;
                    epoll_ctl(server.epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);
                }
            } else {
                handle_client_event((http2_session_t *)events[i].data.ptr, events[i].events);
            }
        }
    }

    close(server.listen_fd);
    close(server.epoll_fd);
    SSL_CTX_free(server.ssl_ctx);
    RETURN_TRUE;
}

static void handle_client_event(http2_session_t *session_data, uint32_t events) {
    if (!session_data->ngh2_session) {
        int ret = SSL_accept(session_data->ssl);
        if (ret == 1) { // Handshake complete
            nghttp2_session_callbacks *callbacks;
            nghttp2_session_callbacks_new(&callbacks);
            nghttp2_session_callbacks_set_send_callback(callbacks, send_callback);
            nghttp2_session_callbacks_set_on_begin_headers_callback(callbacks, on_begin_headers_callback);
            nghttp2_session_callbacks_set_on_header_callback(callbacks, on_header_callback);
            nghttp2_session_callbacks_set_on_data_chunk_recv_callback(callbacks, on_data_chunk_recv_callback);
            nghttp2_session_callbacks_set_on_stream_close_callback(callbacks, on_stream_close_callback);
            nghttp2_session_callbacks_set_on_frame_recv_callback(callbacks, on_frame_recv_callback);
            nghttp2_session_server_new(&session_data->ngh2_session, callbacks, session_data);
            nghttp2_session_callbacks_del(callbacks);
            nghttp2_submit_settings(session_data->ngh2_session, NGHTTP2_FLAG_NONE, NULL, 0);
        } else if (SSL_get_error(session_data->ssl, ret) != SSL_ERROR_WANT_READ && SSL_get_error(session_data->ssl, ret) != SSL_ERROR_WANT_WRITE) {
            close_http2_session(session_data);
            return;
        }
    }

    if (events & EPOLLIN) {
        char buf[READ_BUFFER_SIZE];
        ssize_t bytes_read = SSL_read(session_data->ssl, buf, sizeof(buf));
        if (bytes_read > 0) {
            if (nghttp2_session_mem_recv(session_data->ngh2_session, (const uint8_t*)buf, bytes_read) != bytes_read) {
                close_http2_session(session_data);
                return;
            }
        } else if (bytes_read == 0 || (bytes_read < 0 && SSL_get_error(session_data->ssl, bytes_read) != SSL_ERROR_WANT_READ)) {
            close_http2_session(session_data);
            return;
        }
    }

    if (events & EPOLLOUT) {
        if (nghttp2_session_send(session_data->ngh2_session) != 0) {
            close_http2_session(session_data);
            return;
        }
    }
}

static void close_http2_session(http2_session_t *session) {
    if (session) {
        epoll_ctl(session->server->epoll_fd, EPOLL_CTL_DEL, session->fd, NULL);
        if (session->ngh2_session) nghttp2_session_del(session->ngh2_session);
        if (session->ssl) { SSL_shutdown(session->ssl); SSL_free(session->ssl); }
        if (session->fd >= 0) close(session->fd);
        efree(session);
    }
}

static int on_begin_headers_callback(nghttp2_session *session, const nghttp2_frame *frame, void *user_data) {
    if (frame->hd.type != NGHTTP2_HEADERS || frame->headers.cat != NGHTTP2_HCAT_REQUEST) return 0;
    http2_stream_t *stream_data = ecalloc(1, sizeof(http2_stream_t));
    stream_data->stream_id = frame->hd.stream_id;
    stream_data->session = (http2_session_t *)user_data;
    array_init(&stream_data->request_headers);
    nghttp2_session_set_stream_user_data(session, frame->hd.stream_id, stream_data);
    return 0;
}

static int on_header_callback(nghttp2_session *session, const nghttp2_frame *frame, const uint8_t *name, size_t namelen, const uint8_t *value, size_t valuelen, uint8_t flags, void *user_data) {
    if (frame->hd.type != NGHTTP2_HEADERS || frame->headers.cat != NGHTTP2_HCAT_REQUEST) return 0;
    http2_stream_t *stream_data = nghttp2_session_get_stream_user_data(session, frame->hd.stream_id);
    if (!stream_data) return 0;
    add_assoc_stringl(&stream_data->request_headers, (char*)name, (char*)value, valuelen);
    return 0;
}

static int on_data_chunk_recv_callback(nghttp2_session *session, uint8_t flags, int32_t stream_id, const uint8_t *data, size_t len, void *user_data) {
    http2_stream_t *stream_data = nghttp2_session_get_stream_user_data(session, stream_id);
    if (!stream_data) return 0;
    stream_data->request_body = erealloc(stream_data->request_body, stream_data->request_body_len + len + 1);
    memcpy(stream_data->request_body + stream_data->request_body_len, data, len);
    stream_data->request_body_len += len;
    stream_data->request_body[stream_data->request_body_len] = '\0';
    return 0;
}

static int on_frame_recv_callback(nghttp2_session *session, const nghttp2_frame *frame, void *user_data) {
    if (frame->hd.type != NGHTTP2_HEADERS || !(frame->hd.flags & NGHTTP2_FLAG_END_STREAM)) return 0;
    
    http2_stream_t *stream_data = nghttp2_session_get_stream_user_data(session, frame->hd.stream_id);
    if (!stream_data) return 0;

    zval args[1], retval, request_zv;
    ZVAL_COPY(&request_zv, &stream_data->request_headers);
    if (stream_data->request_body) {
        add_assoc_stringl(&request_zv, "body", stream_data->request_body, stream_data->request_body_len);
    }
    ZVAL_COPY_VALUE(&args[0], &request_zv);

    http2_server_t *server = ((http2_session_t*)user_data)->server;
    server->fci.param_count = 1;
    server->fci.params = args;
    server->fci.retval = &retval;

    if (zend_call_function(&server->fci, &server->fcc) == SUCCESS && Z_TYPE(retval) == IS_ARRAY) {
        zval *status_zv = zend_hash_str_find(Z_ARRVAL(retval), "status", sizeof("status")-1);
        zval *body_zv = zend_hash_str_find(Z_ARRVAL(retval), "body", sizeof("body")-1);
        
        char status_str[4];
        snprintf(status_str, sizeof(status_str), "%ld", status_zv ? zval_get_long(status_zv) : 200);
        
        nghttp2_nv hdrs[] = {
            { (uint8_t*)":status", (uint8_t*)status_str, sizeof(":status")-1, strlen(status_str), NGHTTP2_NV_FLAG_NONE },
            { (uint8_t*)"content-type", (uint8_t*)"text/plain", sizeof("content-type")-1, sizeof("text/plain")-1, NGHTTP2_NV_FLAG_NONE }
        };

        nghttp2_data_provider data_prd;
        if (body_zv && Z_TYPE_P(body_zv) == IS_STRING) {
            stream_data->response_body.data = Z_STRVAL_P(body_zv);
            stream_data->response_body.len = Z_STRLEN_P(body_zv);
            stream_data->response_body.offset = 0;
            data_prd.source.ptr = stream_data;
            data_prd.read_callback = response_read_callback;
        }

        nghttp2_submit_response(session, stream_data->stream_id, hdrs, 2, (body_zv && Z_TYPE_P(body_zv) == IS_STRING) ? &data_prd : NULL);
    }
    
    zval_ptr_dtor(&args[0]);
    zval_ptr_dtor(&retval);
    return 0;
}

static ssize_t response_read_callback(nghttp2_session *session, int32_t stream_id, uint8_t *buf, size_t length, uint32_t *data_flags, nghttp2_data_source *source, void *user_data) {
    http2_stream_t *stream_data = (http2_stream_t *)source->ptr;
    size_t remaining = stream_data->response_body.len - stream_data->response_body.offset;
    size_t to_copy = (length < remaining) ? length : remaining;

    if (to_copy > 0) {
        memcpy(buf, stream_data->response_body.data + stream_data->response_body.offset, to_copy);
        stream_data->response_body.offset += to_copy;
    }

    if (stream_data->response_body.offset == stream_data->response_body.len) {
        *data_flags |= NGHTTP2_DATA_FLAG_EOF;
    }
    
    return to_copy;
}

static int on_stream_close_callback(nghttp2_session *session, int32_t stream_id, uint32_t error_code, void *user_data) {
    http2_stream_t *stream_data = nghttp2_session_get_stream_user_data(session, stream_id);
    if (stream_data) {
        zval_ptr_dtor(&stream_data->request_headers);
        if (stream_data->request_body) efree(stream_data->request_body);
        efree(stream_data);
    }
    return 0;
}

static ssize_t send_callback(nghttp2_session *session, const uint8_t *data, size_t length, int flags, void *user_data) {
    http2_session_t *session_data = (http2_session_t *)user_data;
    ssize_t bytes_written = SSL_write(session_data->ssl, data, length);
    if (bytes_written <= 0) {
        int err = SSL_get_error(session_data->ssl, bytes_written);
        if (err == SSL_ERROR_WANT_WRITE) return NGHTTP2_ERR_WOULDBLOCK;
        return NGHTTP2_ERR_CALLBACK_FAILURE;
    }
    return bytes_written;
}
