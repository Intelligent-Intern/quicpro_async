/* extension/include/server/session.h */

#ifndef QUICPRO_SERVER_SESSION_H
#define QUICPRO_SERVER_SESSION_H

#include <php.h>

/**
 * @file extension/include/server/session.h
 * @brief Public API declarations for server-side session management.
 *
 * This header defines functions for interacting with and managing individual
 * client connections that have been accepted by the server. It provides
 * the necessary tools to inspect connection details, verify client identity
 * in secure contexts, and gracefully terminate sessions from the server-side.
 */

/**
 * @brief Retrieves the verified subject name from a client's TLS certificate.
 *
 * This function is essential for mTLS-based authentication. After a successful
 * handshake where the server has requested and verified a client certificate,
 * this function can be called to extract the subject distinguished name (DN)
 * from that certificate. The application can then use this identity for
 * authorization purposes.
 *
 * @param session_resource The resource representing the authenticated session.
 * @return A PHP string containing the client certificate's subject on success.
 * Returns NULL if the session is not mTLS-authenticated or if no certificate
 * was presented by the client.
 */
PHP_FUNCTION(quicpro_session_get_peer_cert_subject);

/**
 * @brief Initiates a server-side connection termination.
 *
 * This function allows the server to gracefully close a QUIC connection.
 * It sends a `CONNECTION_CLOSE` frame to the client, which includes an
 * application-specific error code and a human-readable reason, providing
 * clear diagnostics to the remote peer.
 *
 * @param session_resource The session resource to be closed.
 * @param error_code An application-defined integer representing the reason for closure.
 * @param reason_str A UTF-8 encoded string detailing the reason for termination.
 * @param reason_len The length of the `reason_str`.
 * @return TRUE on success, indicating the close frame was queued.
 * FALSE on failure (e.g., if the session is already closed).
 */
PHP_FUNCTION(quicpro_session_close_server_initiated);

#endif // QUICPRO_SERVER_SESSION_H
