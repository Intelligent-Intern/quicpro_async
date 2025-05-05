/* include/tls.h  –  TLS Options & Session‑Ticket APIs for php‑quicpro_async
 *
 * This header declares the PHP functions used to configure TLS parameters
 * and manage QUIC session tickets within the php‑quicpro_async extension.
 * By exposing these APIs, userland code can:
 *   • Specify a custom CA bundle for peer certificate verification.
 *   • Provide client certificate and private key files for mutual TLS.
 *   • Export the TLS session ticket from an active QUIC connection
 *     (allowing 0‑RTT resumption).
 *   • Import a previously exported session ticket into a new connection
 *     to resume TLS handshakes quickly.
 */

#ifndef PHP_QUICPRO_TLS_H
#define PHP_QUICPRO_TLS_H

#include "php_quicpro.h"

/* -------------------------------------------------------------------------
 * PHP Function Prototypes
 * ------------------------------------------------------------------------- */

/**
 * quicpro_set_ca_file(string $path): bool
 *
 * Set the global path to the PEM‑formatted CA bundle.  Future calls to
 * quicpro_new_config() will load this file to verify peer certificates.
 * Returns true on success.
 */
PHP_FUNCTION(quicpro_set_ca_file);

/**
 * quicpro_set_client_cert(string $certPath, string $keyPath): bool
 *
 * Set the global client certificate chain and private key files for mTLS.
 * New QUIC configurations will load these files so the extension can
 * perform mutual TLS authentication.  Returns true on success.
 */
PHP_FUNCTION(quicpro_set_client_cert);

/**
 * quicpro_export_session_ticket(resource $session): string
 *
 * Export the most recent TLS session ticket from the given QUIC session.
 * The ticket is returned as a binary string up to QUICPRO_MAX_TICKET_SIZE
 * bytes.  If no ticket is available yet, an empty string is returned.
 */
PHP_FUNCTION(quicpro_export_session_ticket);

/**
 * quicpro_import_session_ticket(resource $session, string $ticket): bool
 *
 * Import a previously exported TLS session ticket into the specified
 * QUIC session.  Allows quiche to resume the TLS handshake in 0‑RTT mode.
 * Returns true on success, or false if the ticket is invalid or rejected.
 */
PHP_FUNCTION(quicpro_import_session_ticket);

#endif /* PHP_QUICPRO_TLS_H */
