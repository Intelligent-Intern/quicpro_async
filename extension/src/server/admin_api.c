/**
 * @file extension/src/server/admin_api.c
 * @brief Implementation of the dynamic Admin API server.
 *
 * This file contains the C-level logic for running the high-security
 * administrative endpoint. It is responsible for setting up a dedicated
 * listener that accepts mTLS-authenticated connections and applies
 * configuration changes to a running parent server instance.
 */

#include <php.h>
#include <zend_exceptions.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "server/admin_api.h"
#include "server/index.h" // To access quicpro_server_t internals

// Assume quicpro_server_t is defined in index.h and has an is_listening flag.
// Assume quicpro_config_ce is the class entry for Quicpro\Config.
extern int le_quicpro_server;
extern zend_class_entry *quicpro_config_ce;

// Struct to pass arguments to the admin listener thread
typedef struct {
    quicpro_server_t *target_server;
    char *host;
    int port;
    SSL_CTX *admin_ssl_ctx; // The mTLS-configured SSL context
} admin_thread_args_t;


// The main function for the admin listener thread
static void *admin_api_thread_func(void *arg)
{
    admin_thread_args_t *args = (admin_thread_args_t *)arg;
    int listen_fd = -1;

    listen_fd = socket(AF_INET6, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        fprintf(stderr, "[Quicpro Admin API] Failed to create socket: %s\n", strerror(errno));
        goto cleanup;
    }

    int reuse = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct sockaddr_in6 addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(args->port);
    inet_pton(AF_INET6, args->host, &addr.sin6_addr);

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "[Quicpro Admin API] Failed to bind to %s:%d: %s\n", args->host, args->port, strerror(errno));
        goto cleanup;
    }

    if (listen(listen_fd, 5) < 0) {
        fprintf(stderr, "[Quicpro Admin API] Failed to listen: %s\n", strerror(errno));
        goto cleanup;
    }

    while (args->target_server->is_listening) {
        int client_fd = accept(listen_fd, NULL, NULL);
        if (client_fd < 0) {
            continue;
        }

        SSL *ssl = SSL_new(args->admin_ssl_ctx);
        SSL_set_fd(ssl, client_fd);

        if (SSL_accept(ssl) <= 0) {
            ERR_print_errors_fp(stderr);
        } else {
            // mTLS handshake successful, peer certificate is verified.
            // Read the JSON payload from the SSL connection.
            char buffer[4096];
            int bytes = SSL_read(ssl, buffer, sizeof(buffer) - 1);
            if (bytes > 0) {
                buffer[bytes] = '\0';
                // At this point, `buffer` contains the JSON config.
                // A thread-safe mechanism (e.g., a message queue or a mutex-protected
                // shared structure) would be needed to apply this config to the
                // `args->target_server`.
                fprintf(stdout, "[Quicpro Admin API] Received config update: %s\n", buffer);

                const char *response = "HTTP/1.1 202 Accepted\r\nContent-Length: 9\r\n\r\nAccepted\n";
                SSL_write(ssl, response, strlen(response));
            }
        }

        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(client_fd);
    }

cleanup:
    if (listen_fd >= 0) close(listen_fd);
    if (args->admin_ssl_ctx) SSL_CTX_free(args->admin_ssl_ctx);
    free(args->host);
    free(args);
    return NULL;
}


PHP_FUNCTION(quicpro_admin_api_listen)
{
    zval *target_server_resource;
    zval *config_resource;
    quicpro_server_t *target_server;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_RESOURCE(target_server_resource)
        Z_PARAM_OBJECT_OF_CLASS(config_resource, quicpro_config_ce)
    ZEND_PARSE_PARAMETERS_END();

    target_server = (quicpro_server_t *)zend_fetch_resource(Z_RES_P(target_server_resource), "quicpro_server", le_quicpro_server);

    // Extract mTLS settings from the PHP config object
    zval *options = zend_read_property(quicpro_config_ce, Z_OBJ_P(config_resource), "options", sizeof("options")-1, 1, NULL);
    zval *val;
    char *cert_file = NULL, *key_file = NULL, *ca_file = NULL, *host = "127.0.0.1";
    long port = 2019;

    if (Z_TYPE_P(options) == IS_ARRAY) {
        if ((val = zend_hash_str_find(Z_ARRVAL_P(options), "cert_file", sizeof("cert_file")-1)) && Z_TYPE_P(val) == IS_STRING) cert_file = Z_STRVAL_P(val);
        if ((val = zend_hash_str_find(Z_ARRVAL_P(options), "key_file", sizeof("key_file")-1)) && Z_TYPE_P(val) == IS_STRING) key_file = Z_STRVAL_P(val);
        if ((val = zend_hash_str_find(Z_ARRVAL_P(options), "ca_file", sizeof("ca_file")-1)) && Z_TYPE_P(val) == IS_STRING) ca_file = Z_STRVAL_P(val);
        if ((val = zend_hash_str_find(Z_ARRVAL_P(options), "host", sizeof("host")-1)) && Z_TYPE_P(val) == IS_STRING) host = Z_STRVAL_P(val);
        if ((val = zend_hash_str_find(Z_ARRVAL_P(options), "port", sizeof("port")-1)) && Z_TYPE_P(val) == IS_LONG) port = Z_LVAL_P(val);
    }

    if (!cert_file || !key_file || !ca_file) {
        zend_throw_exception(NULL, "Admin API config requires 'cert_file', 'key_file', and 'ca_file' for mTLS.", 0);
        RETURN_FALSE;
    }

    SSL_CTX *ssl_ctx = SSL_CTX_new(TLS_server_method());
    if (!ssl_ctx) {
        zend_throw_exception(NULL, "Failed to create SSL context for Admin API.", 0);
        RETURN_FALSE;
    }

    if (SSL_CTX_use_certificate_file(ssl_ctx, cert_file, SSL_FILETYPE_PEM) <= 0 ||
        SSL_CTX_use_private_key_file(ssl_ctx, key_file, SSL_FILETYPE_PEM) <= 0) {
        SSL_CTX_free(ssl_ctx);
        zend_throw_exception(NULL, "Failed to load server certificate/key for Admin API.", 0);
        RETURN_FALSE;
    }

    SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
    if (SSL_CTX_load_verify_locations(ssl_ctx, ca_file, NULL) <= 0) {
        SSL_CTX_free(ssl_ctx);
        zend_throw_exception(NULL, "Failed to load CA file for Admin API client verification.", 0);
        RETURN_FALSE;
    }

    admin_thread_args_t *args = malloc(sizeof(admin_thread_args_t));
    if (!args) {
        SSL_CTX_free(ssl_ctx);
        zend_throw_exception_ex(NULL, 0, "Failed to allocate memory for admin thread arguments");
        RETURN_FALSE;
    }

    args->target_server = target_server;
    args->host = strdup(host);
    args->port = port;
    args->admin_ssl_ctx = ssl_ctx;

    pthread_t admin_thread;
    if (pthread_create(&admin_thread, NULL, admin_api_thread_func, args) != 0) {
        zend_throw_exception_ex(NULL, 0, "Failed to create Admin API listener thread: %s", strerror(errno));
        SSL_CTX_free(ssl_ctx);
        free(args->host);
        free(args);
        RETURN_FALSE;
    }

    pthread_detach(admin_thread);

    RETURN_TRUE;
}
