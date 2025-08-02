/* extension/include/server/tls.h */

#ifndef QUICPRO_SERVER_TLS_H
#define QUICPRO_SERVER_TLS_H

#include <php.h>

/**
 * @file extension/include/server/tls.h
 * @brief Public API declarations for server-side TLS operations.
 *
 * This header defines functions for managing the server's TLS context,
 * with a focus on advanced operational capabilities such as runtime
 * certificate reloading for zero-downtime maintenance.
 */

/**
 * @brief Reloads the TLS certificates and private key for a server instance at runtime.
 *
 * This function enables "zero-downtime" rotation of TLS certificates. When called,
 * the server will load the new certificate and key. All subsequent new connections
 * will use the new credentials, while existing connections remain unaffected and
 * continue with their original TLS context. This is critical for maintaining
 * service availability when certificates expire.
 *
 * @param server_resource The resource of the server instance whose certificates
 * are to be updated.
 * @param cert_file_path The filesystem path to the new PEM-encoded certificate chain file.
 * @param key_file_path The filesystem path to the new PEM-encoded private key file.
 * @return TRUE on success.
 * FALSE on failure, throwing a `Quicpro\Exception\TlsException` if the files
 * cannot be read, are malformed, or if the key does not match the certificate.
 */
PHP_FUNCTION(quicpro_server_reload_tls_config);

#endif // QUICPRO_SERVER_TLS_H
