/*
 * tls.c  –  TLS and session‑ticket management for php‑quicpro_async
 * ----------------------------------------------------------------
 *
 * This file implements all functionality related to managing TLS sessions
 * within our PHP extension that uses quiche for QUIC and HTTP/3.  It
 * provides the ability to export and import session tickets so that
 * clients can perform 0‑RTT resumption, which dramatically reduces
 * connection latency on subsequent connections.  Additionally, it
 * exposes transport‐level statistics and allows global configuration of
 * CA bundles and client certificates for mutual TLS.
 *
 * All functions here are registered as PHP_FUNCTIONs, and their
 * prototypes and arginfo are declared in php_quicpro_arginfo.h.
 * We rely on OpenSSL for stringifying errors and on quiche for
 * the underlying QUIC/TLS operations.
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "php_quicpro.h"               /* Core extension declarations */
#include "php_quicpro_arginfo.h"       /* Arginfo metadata for these functions */

#include <quiche.h>                    /* quiche QUIC + HTTP/3 API */
#include <openssl/ssl.h>               /* OpenSSL SSL/TLS APIs */
#include <openssl/err.h>               /* OpenSSL error-to-string APIs */

/* Forward declaration of the ticket‐ring helper from session.c.
 *
 * We use a simple ring buffer in session.c to store recent TLS
 * session tickets for reuse.  quicpro_ticket_ring_put() appends
 * the latest ticket so that we can later import it into new
 * connections for 0‑RTT resumption.
 */
void quicpro_ticket_ring_put(const uint8_t *ticket, size_t len);


/* ────────────────────────────────────────────────────────────────────────────
 * Global TLS configuration paths
 * ──────────────────────────────────────────────────────────────────────────
 *
 * These global variables hold file paths for CA bundles and client
 * certificates.  They are intended to be used when constructing new
 * quiche_config objects in quicpro_new_config(), so that every new
 * connection picks up the latest user‐provided TLS settings.
 *
 * Note that these globals are process‑wide.  In typical PHP‑FPM
 * or Apache prefork deployments, each process is single‑threaded,
 * so we do not need additional locking.  If embedded in a true
 * multithreaded environment, you would need to serialize access.
 */
static char *g_ca_file   = NULL;
static char *g_cert_file = NULL;
static char *g_key_file  = NULL;


/* ────────────────────────────────────────────────────────────────────────────
 * PHP_FUNCTION(quicpro_export_session_ticket)
 * ────────────────────────────────────────────────────────────────────────────
 *
 * This function exports the most recent TLS session ticket from an active
 * QUIC session into a PHP string.  Having the raw ticket allows userland
 * code to persist it (e.g., in a cache or database) and later re‑import
 * it to resume the TLS handshake in 0‑RTT mode.  If no ticket has been
 * received yet from the server, an empty string is returned.
 *
 * Steps:
 *   1. Parse the session resource parameter.
 *   2. Fetch the internal quicpro_session_t pointer.
 *   3. If s->ticket_len is zero, return an empty string.
 *   4. Otherwise, return the binary ticket as a PHP string.
 *   5. Finally, enqueue the ticket into our ring buffer for future imports.
 */
PHP_FUNCTION(quicpro_export_session_ticket)
{
    zval *z_sess;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_RESOURCE(z_sess)
    ZEND_PARSE_PARAMETERS_END();

    quicpro_session_t *s = zend_fetch_resource(
        Z_RES_P(z_sess), "quicpro", le_quicpro_session
    );
    if (!s) {
        RETURN_THROWS();
    }

    /* If no ticket available yet, return empty string */
    if (s->ticket_len == 0) {
        RETURN_EMPTY_STRING();
    }

    /* Return the raw ticket bytes as a PHP string */
    RETVAL_STRINGL((char *)s->ticket, s->ticket_len);

    /* Store the ticket in our local ring buffer for later reuse */
    quicpro_ticket_ring_put(s->ticket, s->ticket_len);
}


/* ────────────────────────────────────────────────────────────────────────────
 * PHP_FUNCTION(quicpro_import_session_ticket)
 * ────────────────────────────────────────────────────────────────────────────
 *
 * This function imports a previously exported TLS session ticket into
 * an existing or newly created QUIC session.  By injecting the ticket,
 * quiche can perform a 0‑RTT handshake, allowing data to be sent
 * immediately without waiting for the full TLS handshake to complete.
 *
 * Steps:
 *   1. Parse parameters: session resource and ticket string.
 *   2. Validate the ticket length against our maximum buffer.
 *   3. Call quiche_conn_set_session() to inject the ticket.
 *   4. On success, copy the ticket into s->ticket and update s->ticket_len.
 *   5. Enqueue the ticket into our ring buffer as well.
 *
 * Edge cases:
 *   – Empty or oversized blobs trigger a PHP warning and return false.
 *   – If quiche rejects the ticket, we set the extension error and return false.
 */
PHP_FUNCTION(quicpro_import_session_ticket)
{
    zval   *z_sess;
    char   *blob;
    size_t  blob_len;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_RESOURCE(z_sess)
        Z_PARAM_STRING(blob, blob_len)
    ZEND_PARSE_PARAMETERS_END();

    quicpro_session_t *s = zend_fetch_resource(
        Z_RES_P(z_sess), "quicpro", le_quicpro_session
    );
    if (!s) {
        RETURN_THROWS();
    }

    /* Sanity check: ticket must fit in our session buffer */
    if (blob_len == 0 || blob_len > sizeof s->ticket) {
        php_error_docref(NULL, E_WARNING,
                         "session ticket length (%zu) out of bounds", blob_len);
        RETURN_FALSE;
    }

    /* Inject the ticket into the QUIC/TLS handshake context */
    int rc = quiche_conn_set_session(s->conn,
                                     (const uint8_t *)blob,
                                     blob_len);
    if (rc < 0) {
        quicpro_set_error("libquiche rejected the session ticket");
        RETURN_FALSE;
    }

    /* Copy the ticket locally for future export or diagnostics */
    memcpy(s->ticket, blob, blob_len);
    s->ticket_len = blob_len;

    /* Also enqueue into the local ring buffer */
    quicpro_ticket_ring_put(s->ticket, s->ticket_len);

    RETURN_TRUE;
}


/* ────────────────────────────────────────────────────────────────────────────
 * PHP_FUNCTION(quicpro_get_stats)
 * ────────────────────────────────────────────────────────────────────────────
 *
 * This function returns a comprehensive set of transport‑level metrics
 * for the active QUIC connection in a PHP associative array.  Metrics
 * include packet counters (sent, received, lost), the current RTT, and
 * the congestion window size.  These values can be used for monitoring
 * performance, debugging, or dynamic traffic shaping.
 *
 * Steps:
 *   1. Parse the session resource parameter.
 *   2. Fetch quicpro_session_t from the resource.
 *   3. Call quiche_conn_stats() to get per‑packet counters.
 *   4. Call quiche_conn_path_stats() for RTT and cwnd.
 *   5. Populate a PHP array with these metrics and return it.
 */
PHP_FUNCTION(quicpro_get_stats)
{
    zval *z_sess;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_RESOURCE(z_sess)
    ZEND_PARSE_PARAMETERS_END();

    quicpro_session_t *s = zend_fetch_resource(
        Z_RES_P(z_sess), "quicpro", le_quicpro_session
    );
    if (!s) {
        RETURN_THROWS();
    }

    /* Gather packet-level counters */
    quiche_stats qs;
    quiche_conn_stats(s->conn, &qs);

    /* Gather path-specific stats (RTT, cwnd) on path index 0 */
    quiche_path_stats ps;
    quiche_conn_path_stats(s->conn, 0, &ps);

    /* Build the PHP associative array with all collected metrics */
    array_init(return_value);
    add_assoc_long(return_value, "pkt_rx", (zend_long)qs.recv);
    add_assoc_long(return_value, "pkt_tx", (zend_long)qs.sent);
    add_assoc_long(return_value, "lost",   (zend_long)qs.lost);
    add_assoc_long(return_value, "rtt_ns", (zend_long)ps.rtt);
    add_assoc_long(return_value, "cwnd",   (zend_long)ps.cwnd);
}


/* ────────────────────────────────────────────────────────────────────────────
 * PHP_FUNCTION(quicpro_set_ca_file)
 * ────────────────────────────────────────────────────────────────────────────
 *
 * Sets the global path to the PEM‑formatted CA bundle.  This bundle
 * will be loaded by subsequent calls to quicpro_new_config(), enabling
 * peer certificate verification based on user‑provided trust anchors.
 *
 * Steps:
 *   1. Parse one string parameter for the file path.
 *   2. Free any previously stored path.
 *   3. Duplicate the new path into g_ca_file.
 *   4. Return true to indicate success.
 *
 * Note:
 *   This change affects only new configurations created after this call.
 */
PHP_FUNCTION(quicpro_set_ca_file)
{
    char   *path;
    size_t  path_len;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STRING(path, path_len)
    ZEND_PARSE_PARAMETERS_END();

    efree(g_ca_file);
    g_ca_file = estrndup(path, path_len);

    RETURN_TRUE;
}


/* ────────────────────────────────────────────────────────────────────────────
 * PHP_FUNCTION(quicpro_set_client_cert)
 * ────────────────────────────────────────────────────────────────────────────
 *
 * Sets the global paths for the client certificate chain and private key.
 * These files will be loaded into future quiche_config instances to
 * enable mutual TLS (mTLS) when connecting to servers that require it.
 *
 * Steps:
 *   1. Parse two string parameters: cert file path and key file path.
 *   2. Free any previously stored paths.
 *   3. Duplicate both new paths into g_cert_file and g_key_file.
 *   4. Return true to indicate success.
 *
 * Note:
 *   Only new configurations will pick up these values; existing
 *   connections remain unaffected.
 */
PHP_FUNCTION(quicpro_set_client_cert)
{
    char   *cert, *key;
    size_t  cert_len, key_len;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STRING(cert, cert_len)
        Z_PARAM_STRING(key,  key_len)
    ZEND_PARSE_PARAMETERS_END();

    efree(g_cert_file);
    efree(g_key_file);
    g_cert_file = estrndup(cert,  cert_len);
    g_key_file  = estrndup(key,   key_len);

    RETURN_TRUE;
}


/* ────────────────────────────────────────────────────────────────────────────
 * Function table for TLS-related PHP functions
 * ────────────────────────────────────────────────────────────────────────────
 *
 * This array maps the PHP function names to their C implementations
 * along with their automatically generated arginfo metadata.  It is
 * registered in the extension's MINIT function so that PHP knows how
 * to call these methods and check their parameter types.
 */
const zend_function_entry quicpro_tls_functions[] = {
    PHP_FE(quicpro_export_session_ticket,  arginfo_quicpro_export_session_ticket)
    PHP_FE(quicpro_import_session_ticket,  arginfo_quicpro_import_session_ticket)
    PHP_FE(quicpro_get_stats,              arginfo_quicpro_get_stats)
    PHP_FE(quicpro_set_ca_file,            arginfo_quicpro_set_ca_file)
    PHP_FE(quicpro_set_client_cert,        arginfo_quicpro_set_client_cert)
    PHP_FE_END
};
