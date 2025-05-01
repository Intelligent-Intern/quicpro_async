/*─────────────────────────────────────────────────────────────────────────────
 *  tls.c  —  TLS-related helpers for php-quicpro
 *─────────────────────────────────────────────────────────────────────────────
 *
 *  The code in this file sits between OpenSSL / libquiche and user-land PHP.
 *  Modern QUIC relies on TLS 1.3 for encryption and early-data resumption,
 *  so sooner or later every connection needs to touch the routines you see
 *  here.  We expose three public PHP functions: one to export the resumption
 *  ticket, one to import it, and one to fetch run-time statistics such as
 *  round-trip time.  Two additional helpers let scripts specify a CA bundle
 *  and load a client certificate without recompiling the extension.
 */

#include "php_quicpro.h"      /* quicpro_set_error(), resource IDs        */
#include <quiche.h>           /* quiche_conn_stats(), quiche_conn_*       */

#include <openssl/ssl.h>      /* file-based certificate helpers           */
#include <errno.h>
#include <string.h>
#include <unistd.h>

/* The actual quicpro_session_t definition lives in php_quicpro.h;   */
/* we only need a prototype for this helper that lives in session.c.  */
void quicpro_ticket_ring_put(const uint8_t *ticket, size_t len);

/* Global ticket ring
 *
 * A single worker process can open hundreds of connections per second.  To
 * avoid a full handshake each time, servers hand out “session tickets”.  The
 * next connection can send that ticket and jump straight to encrypted data,
 * saving one round-trip.  We store tickets in a lock-free ring buffer that
 * any thread in the same process can read.  A pointer lives here so both
 * session.c (producer) and this file (consumer) look at the exact same ring.
 */
quicpro_ticket_ring_t *g_ticket_ring = NULL;

/* Compile-time guard
 *
 * The struct in php_quicpro.h contains a fixed-size byte array named
 * “ticket”.  This _Static_assert verifies at compilation that the array is
 * large enough for every ticket a server is allowed to send according to
 * QUICPRO_MAX_TICKET_SIZE.  If someone changes either constant in the future
 * and forgets to update the other, the build fails with a descriptive error.
 */
_Static_assert(sizeof(((quicpro_session_t *)0)->ticket) == QUICPRO_MAX_TICKET_SIZE,
               "ticket buffer too small for maximum TLS ticket size");

/* quicpro_export_session_ticket
 *
 * PHP calls this after the TLS handshake.  If libquiche has already generated
 * a resumption ticket we copy it into a PHP string and return it to the
 * caller.  We also push a duplicate into the shared ring so completely
 * unrelated sessions inside the same FPM pool can skip their own handshakes.
 */
PHP_FUNCTION(quicpro_export_session_ticket)
{
    zval *z_sess;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_RESOURCE(z_sess)
    ZEND_PARSE_PARAMETERS_END();

    quicpro_session_t *s =
        zend_fetch_resource(Z_RES_P(z_sess), "quicpro", le_quicpro_session);
    if (!s) RETURN_THROWS();

    if (s->ticket_len == 0) {
        /* No ticket yet — the handshake is probably still in progress. */
        RETURN_EMPTY_STRING();
    }

    /* Copy to PHP first so the caller gets data even if the ring write fails. */
    RETVAL_STRINGL((char *)s->ticket, s->ticket_len);

    /* Best-effort publish into the ring.  Failure is not fatal.               */
    quicpro_ticket_ring_put(s->ticket, s->ticket_len);
}

/* quicpro_import_session_ticket
 *
 * When a script has a fresh ticket — from the ring, a database, or a previous
 * process — it can feed the blob back into libquiche.  Passing the ticket
 * before the handshake lets the library attempt 0-RTT immediately, reducing
 * latency for the end-user.  We memorise the blob inside the session struct
 * so scripts can retrieve the exact same data later for debugging.
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

    quicpro_session_t *s =
        zend_fetch_resource(Z_RES_P(z_sess), "quicpro", le_quicpro_session);
    if (!s) RETURN_THROWS();

    if (blob_len == 0 || blob_len > sizeof s->ticket) {
        php_error_docref(NULL, E_WARNING,
                         "session ticket length (%zu) out of bounds", blob_len);
        RETURN_FALSE;
    }

    if (quiche_conn_set_tls_ticket(s->conn,
                                   (uint8_t *)blob,
                                   blob_len) < 0) {
        quicpro_set_error("libquiche rejected the session ticket");
        RETURN_FALSE;
    }

    memcpy(s->ticket, blob, blob_len);
    s->ticket_len = blob_len;
    quicpro_ticket_ring_put(s->ticket, s->ticket_len);
    RETURN_TRUE;
}

/* quicpro_get_stats
 *
 * libquiche maintains counters for every connection: packets sent, packets
 * received, lost packets, current round-trip time, and the size of the
 * congestion window.  This helper copies the numbers into an associative
 * PHP array so application code can log or graph them at will.
 */
PHP_FUNCTION(quicpro_get_stats)
{
    zval *z_sess;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_RESOURCE(z_sess)
    ZEND_PARSE_PARAMETERS_END();

    quicpro_session_t *s =
        zend_fetch_resource(Z_RES_P(z_sess), "quicpro", le_quicpro_session);
    if (!s) RETURN_THROWS();

    quiche_stats qs;
    quiche_conn_stats(s->conn, &qs);

    array_init(return_value);
    add_assoc_long(return_value, "pkt_rx", qs.recv);
    add_assoc_long(return_value, "pkt_tx", qs.sent);
    add_assoc_long(return_value, "lost",   qs.lost);
    add_assoc_long(return_value, "rtt_ns", qs.rtt);
    add_assoc_long(return_value, "cwnd",   qs.cwnd);
}

/* quicpro_set_ca_file
 *
 * A PHP script may want to override the default trust store, for instance
 * when talking to an internal server signed by a private CA.  The path is
 * passed straight to libquiche, which validates that the bundle exists and
 * is readable.  We translate any error into quicpro_last_error.
 */
PHP_FUNCTION(quicpro_set_ca_file)
{
    char   *path;
    size_t  path_len;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STRING(path, path_len)
    ZEND_PARSE_PARAMETERS_END();

    if (quiche_set_ca_cert_file(path) != 0) {
        quicpro_set_error("unable to load CA bundle");
        RETURN_FALSE;
    }
    RETURN_TRUE;
}

/* quicpro_set_client_cert
 *
 * Mutual TLS is optional in QUIC but some APIs require it.  The function
 * accepts a PEM-encoded certificate chain and a matching private key.  Both
 * files must exist on disk because libquiche hands the filenames to OpenSSL.
 * On success future connections created from this PHP process present the
 * certificate automatically.
 */
PHP_FUNCTION(quicpro_set_client_cert)
{
    char   *crt; size_t crt_len;
    char   *key; size_t key_len;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STRING(crt, crt_len)
        Z_PARAM_STRING(key, key_len)
    ZEND_PARSE_PARAMETERS_END();

    if (quiche_set_certificate_chain_file(crt) != 0 ||
        quiche_set_private_key_file(key) != 0) {
        quicpro_set_error("failed to load client certificate or key");
        RETURN_FALSE;
    }
    RETURN_TRUE;
}

/* Zend function table
 *
 * Internally the PHP engine relies on a C array that maps plain C functions
 * to their script-visible names.  Each entry also references an “arginfo”
 * block so PHP can enforce type hints before control ever reaches C land.
 * Registering the table during module start-up is what makes the five helper
 * functions above callable from regular `<?php ?>` code.
 */
const zend_function_entry quicpro_tls_functions[] = {
    PHP_FE(quicpro_export_session_ticket,  arginfo_quicpro_export_session_ticket)
    PHP_FE(quicpro_import_session_ticket,  arginfo_quicpro_import_session_ticket)
    PHP_FE(quicpro_get_stats,              arginfo_quicpro_get_stats)
    PHP_FE(quicpro_set_ca_file,            arginfo_quicpro_set_ca_file)
    PHP_FE(quicpro_set_client_cert,        arginfo_quicpro_set_client_cert)
    PHP_FE_END
};
