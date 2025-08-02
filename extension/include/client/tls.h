#ifndef QUICPRO_CLIENT_TLS_H
#define QUICPRO_CLIENT_TLS_H

#include <php.h> // Essential for PHP_FUNCTION macro and fundamental PHP types.

/**
 * @file extension/include/client/tls.h
 * @brief Client-Side TLS and Cryptography Management for Quicpro.
 *
 * This header defines the public API for managing TLS (Transport Layer Security)
 * and associated cryptographic aspects specifically from the client perspective
 * within the Quicpro extension. It provides essential functions for configuring
 * secure communication channels, leveraging OpenSSL and `quiche`'s TLS stack.
 *
 * This module allows for fine-grained control over client-side TLS behavior,
 * including peer authentication, client certificate provisioning for mutual TLS (mTLS),
 * and the efficient reuse of TLS sessions through session tickets to enable 0-RTT
 * (Zero Round-Trip Time) connection resumptions. It consolidates TLS-related
 * functionalities previously found in the legacy `tls.h` (client-side specific parts).
 */

/**
 * @brief Sets the default CA (Certificate Authority) file for client-side peer verification.
 *
 * This function configures a global or default path to a CA certificate bundle
 * file in PEM format. This CA file is used by client-side TLS connections
 * (both QUIC-based HTTP/3 and TCP-based HTTP/1.1/2) to verify the authenticity
 * of server certificates during the TLS handshake.
 * Providing a trusted CA file is crucial for secure communication, ensuring
 * that the client only connects to legitimate servers. This setting can
 * often be overridden by more specific configurations per request.
 *
 * @param path_str The null-terminated string path to the CA certificate file. Mandatory.
 * @param path_len The length of `path_str`.
 * @return TRUE on success. Returns FALSE on failure (e.g., file not found, permission issues).
 * Throws a `Quicpro\Exception\TlsException` for relevant errors.
 */
PHP_FUNCTION(quicpro_client_tls_set_ca_file);

/**
 * @brief Sets the default client certificate and private key files for mTLS.
 *
 * This function configures the default paths to the client's public certificate
 * file and its corresponding private key file. These are used when performing
 * mutual TLS (mTLS) authentication, where the client needs to present its own
 * certificate to the server for verification.
 * This is critical for establishing trusted connections in environments
 * requiring strong client identity verification. This setting can also
 * typically be overridden per-connection or per-request.
 *
 * @param cert_str The null-terminated string path to the client's certificate file (e.g., in PEM format). Mandatory.
 * @param cert_len The length of `cert_str`.
 * @param key_str The null-terminated string path to the client's private key file (e.g., in PEM format). Mandatory.
 * @param key_len The length of `key_str`.
 * @return TRUE on success. Returns FALSE on failure (e.g., file not found, invalid format, permission issues).
 * Throws a `Quicpro\Exception\TlsException`.
 */
PHP_FUNCTION(quicpro_client_tls_set_client_cert);

/**
 * @brief Exports the latest TLS session ticket from an established QUIC client session.
 *
 * After a successful QUIC/TLS handshake, the server may provide a TLS session ticket.
 * This function extracts that binary ticket from the `Quicpro\Session` resource.
 * Session tickets are vital for enabling 0-RTT (Zero Round-Trip Time) connection
 * resumption for subsequent connections to the same server, significantly reducing
 * handshake latency. The exported ticket can be stored by the application
 * (e.g., in a cache or database).
 *
 * @param session_resource A PHP resource representing the `Quicpro\Session` from which to export the ticket. Mandatory.
 * @return A PHP string containing the raw binary session ticket on success.
 * Returns an empty string if the session is not established or no ticket is available.
 * Returns FALSE on error, throwing a `Quicpro\Exception\TlsException`.
 */
PHP_FUNCTION(quicpro_client_tls_export_session_ticket);

/**
 * @brief Imports a previously exported TLS session ticket into a new or unestablished QUIC client session.
 *
 * This function allows an application to import a raw binary TLS session ticket
 * into a new QUIC session *before* its handshake is completed. If the imported
 * ticket is valid and accepted by the server, it can enable a 0-RTT handshake.
 * This means the client can send application data immediately with its first
 * flight of packets, drastically reducing latency.
 *
 * @param session_resource A PHP resource representing the `Quicpro\Session` into which to import the ticket.
 * The session must be in an unestablished state (e.g., immediately after `quicpro_client_session_connect`
 * but before handshake completion). Mandatory.
 * @param ticket_blob_str The raw binary string of the TLS session ticket to import. Mandatory.
 * @param ticket_blob_len The length of `ticket_blob_str`.
 * @return TRUE on successful import of the ticket. Returns FALSE on failure
 * (e.g., invalid ticket format, ticket rejected by `quiche`).
 * Throws a `Quicpro\Exception\TlsException`.
 */
PHP_FUNCTION(quicpro_client_tls_import_session_ticket);

#endif // QUICPRO_CLIENT_TLS_H