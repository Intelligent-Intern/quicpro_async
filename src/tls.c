/*  TLS helpers & session‑ticket handling for php‑quicpro
 *  ----------------------------------------------------------------
 *  Implements ticket import/export, CA / client‑cert helpers and pushes
 *  each freshly issued or successfully restored ticket into the shared
 *  memory ring‑buffer for observability.  Requires functions provided in
 *  config.c (quicpro_fetch_config) and ring.c (quicpro_ticket_ring_put).
 */

#include "php_quicpro.h"

#include <string.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <assert.h>

/* -------------------------------------------------------------------------
 *  Compile‑time sanity check – the quicpro_session_t struct in php_quicpro.h
 *  must contain a 512‑byte ticket buffer.
 * ------------------------------------------------------------------------- */
_Static_assert(sizeof(((quicpro_session_t *)0)->ticket) == 512,
               "TLS ticket buffer must be exactly 512 bytes");

/* -------------------------------------------------------------------------
 *  Thread‑local error buffer – reset at entry of each public function.
 * ------------------------------------------------------------------------- */
static __thread char last_error[256];
static inline void quicpro_reset_error(void) { last_error[0] = '\0'; }

/* shared‑memory trace helpers (ring.c) ----------------------------------- */
extern void quicpro_ticket_ring_put(const uint8_t *ticket, size_t len);

/* config‑helper (config.c) ------------------------------------------------ */
extern quiche_config *quicpro_fetch_config(zval *zcfg);

/* =========================================================================
 *  Session‑ticket helpers
 * ========================================================================= */

PHP_FUNCTION(quicpro_export_session_ticket)
{
    quicpro_reset_error();

    zval *zsess;
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "r", &zsess) == FAILURE) {
        RETURN_EMPTY_STRING();
    }

    quicpro_session_t *s = zend_fetch_resource(Z_RES_P(zsess), "quicpro", le_quicpro);
    if (!s || s->ticket_len == 0) {
        RETURN_EMPTY_STRING();
    }

    /* also mirror to SHM trace */
    quicpro_ticket_ring_put(s->ticket, s->ticket_len);

    RETURN_STRINGL((char *)s->ticket, s->ticket_len);
}

PHP_FUNCTION(quicpro_import_session_ticket)
{
    quicpro_reset_error();

    zval *zsess;
    char *ticket;
    size_t ticket_len;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "rs", &zsess, &ticket, &ticket_len) == FAILURE) {
        RETURN_FALSE;
    }

    quicpro_session_t *s = zend_fetch_resource(Z_RES_P(zsess), "quicpro", le_quicpro);
    if (!s || ticket_len == 0 || ticket_len > sizeof(s->ticket)) {
        RETURN_FALSE;
    }

    /* first ask quiche to accept the ticket – copy only on success */
    if (quiche_conn_set_tls_ticket(s->conn, (const uint8_t *)ticket, ticket_len) != 0) {
        snprintf(last_error, sizeof(last_error), "Failed to import TLS session ticket");
        RETURN_FALSE;
    }

    memcpy(s->ticket, ticket, ticket_len);
    s->ticket_len = ticket_len;
    quicpro_ticket_ring_put(s->ticket, s->ticket_len);

    RETURN_TRUE;
}

/* =========================================================================
 *  quiche_config helpers – expect a QuicConfig resource as first argument.
 * ========================================================================= */

PHP_FUNCTION(quicpro_set_ca_file)
{
    quicpro_reset_error();

    zval *zcfg;
    char *cafile;
    size_t cafile_len;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "rs", &zcfg, &cafile, &cafile_len) == FAILURE) {
        RETURN_FALSE;
    }

    quiche_config *cfg = quicpro_fetch_config(zcfg);
    if (!cfg) RETURN_FALSE;

    if (quiche_config_load_verify_locations_from_file(cfg, cafile) != 0) {
        snprintf(last_error, sizeof(last_error), "Failed to load CA file");
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

PHP_FUNCTION(quicpro_set_client_cert)
{
    quicpro_reset_error();

    zval *zcfg;
    char *cert, *key;
    size_t cert_len, key_len;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "rss", &zcfg, &cert, &cert_len, &key, &key_len) == FAILURE) {
        RETURN_FALSE;
    }

    quiche_config *cfg = quicpro_fetch_config(zcfg);
    if (!cfg) RETURN_FALSE;

    if (quiche_config_load_cert_chain_from_pem_file(cfg, cert) != 0 ||
        quiche_config_load_priv_key_from_pem_file(cfg, key) != 0) {
        snprintf(last_error, sizeof(last_error), "Failed to load client cert or key");
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

/* =========================================================================
 *  Diagnostics & stats
 * ========================================================================= */

PHP_FUNCTION(quicpro_get_last_error)
{
    RETURN_STRING(last_error);
}

PHP_FUNCTION(quicpro_get_stats)
{
    quicpro_reset_error();

    zval *zsess;
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "r", &zsess) == FAILURE) {
        array_init(return_value);
        return;
    }

    quicpro_session_t *s = zend_fetch_resource(Z_RES_P(zsess), "quicpro", le_quicpro);
    if (!s) {
        array_init(return_value);
        return;
    }

    quiche_stats stats;
    quiche_conn_stats(s->conn, &stats);

    array_init(return_value);
    add_assoc_long(return_value, "recv",  stats.recv);
    add_assoc_long(return_value, "sent",  stats.sent);
    add_assoc_long(return_value, "lost",  stats.lost);
    add_assoc_long(return_value, "rtt",   stats.rtt);
    add_assoc_long(return_value, "cwnd",  stats.cwnd);
#ifdef QUICHE_STATS_VER2
    add_assoc_long(return_value, "total_recv",  stats.total_recv);
    add_assoc_long(return_value, "total_sent",  stats.total_sent);
#endif
}

PHP_FUNCTION(quicpro_version)
{
    RETURN_STRING("0.1.0-dev");
}
