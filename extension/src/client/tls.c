#include "php_quicpro.h"
#include "client/tls.h"
#include "client/session.h" // For quicpro_session_t and session resource handling
#include "config.h" // For quicpro_cfg_t and config resource handling
#include "cancel.h" // For exception throwing

#include <openssl/ssl.h> // For SSL_CTX and SSL_library_init
#include <openssl/err.h> // For ERR_print_errors_fp
#include <string.h> // For strncpy

/**
 * @file extension/src/client/tls.c
 * @brief Client-Side TLS and Cryptography Management Implementation for Quicpro.
 *
 * This file implements the PHP-callable functions declared in `client/tls.h`,
 * providing a comprehensive set of functionalities for managing TLS (Transport Layer Security)
 * and associated cryptographic aspects from the client perspective.
 *
 * It acts as the bridge between PHP userland and the underlying OpenSSL and `quiche` TLS stack,
 * allowing for configuration of Certificate Authority (CA) files for server verification,
 * client certificates for mutual TLS (mTLS) authentication, and the efficient
 * management of TLS session tickets for 0-RTT (Zero Round-Trip Time) connection resumption.
 *
 * The implementation ensures secure and performant client-side communication by
 * correctly configuring the `quiche_config` object's TLS settings and by handling
 * the lifecycle of TLS session tickets.
 */

/**
 * @brief Sets the default CA (Certificate Authority) file for client-side peer verification.
 *
 * This PHP function configures the global or default path to a CA certificate bundle
 * file. This file is critical for validating the authenticity of server certificates
 * during the TLS handshake, preventing man-in-the-middle attacks. The path provided
 * will be used by subsequent `Quicpro\Config` objects unless overridden.
 * Internally, it updates the `tls_default_ca_file` setting within the global
 * Quicpro TLS configuration.
 *
 * @param path_str The null-terminated string path to the CA certificate file in PEM format.
 * @param path_len The length of `path_str`.
 * @return TRUE on successful setting of the CA file path. Returns FALSE on failure
 * if the path is invalid or cannot be accessed, throwing a `Quicpro\Exception\TlsException`.
 */
PHP_FUNCTION(quicpro_client_tls_set_ca_file) {
    char *path_str;
    size_t path_len;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STRING(path_str, path_len)
    ZEND_PARSE_PARAMETERS_END();

    // The actual storage and application of this setting happens via the
    // global configuration mechanism (qp_config_tls_crypto_config.tls_default_ca_file).
    // The validation function will handle allocation and copying.
    if (qp_config_tls_crypto_apply_userland_config_entry("tls_default_ca_file", path_str, path_len) == FAILURE) {
        // qp_config_tls_crypto_apply_userland_config_entry throws exception on failure
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

/**
 * @brief Sets the default client certificate and private key files for mutual TLS (mTLS).
 *
 * This PHP function allows applications to specify their default client certificate
 * and corresponding private key. These files are used for mTLS, where the client
 * authenticates itself to the server using its certificate. The paths provided
 * will be used by subsequent `Quicpro\Config` objects unless overridden.
 * Internally, it updates `tls_default_cert_file` and `tls_default_key_file`
 * in the global Quicpro TLS configuration.
 *
 * @param cert_str The null-terminated string path to the client's certificate file (e.g., in PEM format).
 * @param cert_len The length of `cert_str`.
 * @param key_str The null-terminated string path to the client's private key file (e.g., in PEM format).
 * @param key_len The length of `key_str`.
 * @return TRUE on successful setting of the certificate and key file paths. Returns FALSE on failure
 * if paths are invalid or inaccessible, throwing a `Quicpro\Exception\TlsException`.
 */
PHP_FUNCTION(quicpro_client_tls_set_client_cert) {
    char *cert_str;
    size_t cert_len;
    char *key_str;
    size_t key_len;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STRING(cert_str, cert_len)
        Z_PARAM_STRING(key_str, key_len)
    ZEND_PARSE_PARAMETERS_END();

    // Apply certificate file path
    if (qp_config_tls_crypto_apply_userland_config_entry("tls_default_cert_file", cert_str, cert_len) == FAILURE) {
        RETURN_FALSE;
    }
    // Apply key file path
    if (qp_config_tls_crypto_apply_userland_config_entry("tls_default_key_file", key_str, key_len) == FAILURE) {
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

/**
 * @brief Exports the latest TLS session ticket from an established QUIC client session.
 *
 * This PHP function retrieves the binary TLS session ticket from an active
 * `Quicpro\Session` resource. Session tickets are vital for enabling 0-RTT
 * (Zero Round-Trip Time) connection resumption, which significantly reduces
 * handshake latency for subsequent connections to the same server. The exported
 * ticket can be persisted and later imported into new sessions.
 *
 * @param session_resource A PHP resource representing the `Quicpro\Session` from which to export the ticket.
 * @return A PHP string containing the raw binary session ticket on success.
 * Returns an empty string if the session is not established or no ticket is available.
 * Returns FALSE on error, throwing a `Quicpro\Exception\TlsException`.
 */
PHP_FUNCTION(quicpro_client_tls_export_session_ticket) {
    zval *z_sess_res;
    quicpro_session_t *s;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_RESOURCE(z_sess_res)
    ZEND_PARSE_PARAMETERS_END();

    // Fetch the underlying quicpro_session_t from the PHP resource.
    s = (quicpro_session_t *)zend_fetch_resource_ex(z_sess_res, "Quicpro\\Session", le_quicpro_session);
    if (!s || s->conn == NULL) {
        throw_tls_exception(0, "Invalid or uninitialized Quicpro\\Session resource provided.");
        RETURN_FALSE;
    }

    // Check if a session ticket is available.
    if (s->ticket_len == 0) {
        RETURN_EMPTY_STRING(); // No ticket available, return empty string.
    }

    // Return the session ticket as a PHP string.
    RETVAL_STRINGL((char *)s->ticket, s->ticket_len);
}

/**
 * @brief Imports a previously exported TLS session ticket into a new or unestablished QUIC client session.
 *
 * This PHP function allows an application to import a raw binary TLS session ticket
 * into a new `Quicpro\Session` resource before the QUIC/TLS handshake completes.
 * Successful import can enable a 0-RTT handshake, allowing the client to send
 * application data immediately with its first flight of packets, significantly
 * reducing connection setup latency.
 *
 * @param session_resource A PHP resource representing the `Quicpro\Session` into which to import the ticket.
 * The session should typically be freshly created and not yet established.
 * @param ticket_blob_str The raw binary string of the TLS session ticket to import.
 * @param ticket_blob_len The length of `ticket_blob_str`.
 * @return TRUE on successful import of the ticket. Returns FALSE on failure
 * (e.g., invalid ticket format, ticket rejected by `quiche`, session already established).
 * Throws a `Quicpro\Exception\TlsException`.
 */
PHP_FUNCTION(quicpro_client_tls_import_session_ticket) {
    zval *z_sess_res;
    char *ticket_blob_str;
    size_t ticket_blob_len;
    quicpro_session_t *s;
    int rc;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_RESOURCE(z_sess_res)
        Z_PARAM_STRING(ticket_blob_str, ticket_blob_len)
    ZEND_PARSE_PARAMETERS_END();

    // Fetch the underlying quicpro_session_t from the PHP resource.
    s = (quicpro_session_t *)zend_fetch_resource_ex(z_sess_res, "Quicpro\\Session", le_quicpro_session);
    if (!s || s->conn == NULL) {
        throw_tls_exception(0, "Invalid or uninitialized Quicpro\\Session resource provided.");
        RETURN_FALSE;
    }

    // Validate ticket length. It must fit within the predefined buffer size.
    if (ticket_blob_len == 0 || ticket_blob_len > sizeof(s->ticket)) {
        throw_tls_exception(0, "Session ticket length (%zu) is out of bounds or zero. Max allowed: %zu bytes.", ticket_blob_len, sizeof(s->ticket));
        RETURN_FALSE;
    }

    // Attempt to set the session ticket in the quiche connection.
    rc = quiche_conn_set_session(s->conn, (const uint8_t *)ticket_blob_str, ticket_blob_len);
    if (rc < 0) {
        throw_tls_exception(rc, "libquiche rejected the provided session ticket with error code %d.", rc);
        RETURN_FALSE;
    }

    // If successfully set, store the ticket locally in the session struct for future reference.
    memcpy(s->ticket, ticket_blob_str, ticket_blob_len);
    s->ticket_len = (uint32_t)ticket_blob_len;

    RETURN_TRUE;
}