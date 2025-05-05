/* extension/src/tls.c */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "php_quicpro.h"
#include <quiche.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <errno.h>
#include <unistd.h>

/* Ticket‐ring helper (from session.c) */
void quicpro_ticket_ring_put(const uint8_t *ticket, size_t len);

/* ─── Global paths for CA bundle and client certificates ─────────────── */
static char *g_ca_file   = NULL;
static char *g_cert_file = NULL;
static char *g_key_file  = NULL;

/* ─── quicpro_export_session_ticket ──────────────────────────────────── */
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
        RETURN_EMPTY_STRING();
    }

    RETVAL_STRINGL((char *)s->ticket, s->ticket_len);
    quicpro_ticket_ring_put(s->ticket, s->ticket_len);
}

/* ─── quicpro_import_session_ticket ──────────────────────────────────── */
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

    /* renamed in quiche: quiche_conn_set_session */
    int rc = quiche_conn_set_session(s->conn,
                                     (const uint8_t *)blob,
                                     blob_len);
    if (rc < 0) {
        quicpro_set_error("libquiche rejected the session ticket");
        RETURN_FALSE;
    }

    memcpy(s->ticket, blob, blob_len);
    s->ticket_len = blob_len;
    quicpro_ticket_ring_put(s->ticket, s->ticket_len);
    RETURN_TRUE;
}

/* ─── quicpro_get_stats ───────────────────────────────────────────────── */
PHP_FUNCTION(quicpro_get_stats)
{
    zval *z_sess;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_RESOURCE(z_sess)
    ZEND_PARSE_PARAMETERS_END();

    quicpro_session_t *s =
        zend_fetch_resource(Z_RES_P(z_sess), "quicpro", le_quicpro_session);
    if (!s) RETURN_THROWS();

    /* packet‐level counters */
    quiche_stats qs;
    quiche_conn_stats(s->conn, &qs);

    /* path‐level stats (rtt, cwnd) moved to quiche_path_stats */
    quiche_path_stats ps;
    quiche_conn_path_stats(s->conn, 0, &ps);

    array_init(return_value);
    add_assoc_long(return_value, "pkt_rx", (zend_long)qs.recv);
    add_assoc_long(return_value, "pkt_tx", (zend_long)qs.sent);
    add_assoc_long(return_value, "lost",   (zend_long)qs.lost);
    /* use path_stats for RTT and CWND */
    add_assoc_long(return_value, "rtt_ns", (zend_long)ps.rtt);
    add_assoc_long(return_value, "cwnd",   (zend_long)ps.cwnd);
}

/* ─── quicpro_set_ca_file ─────────────────────────────────────────────── */
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

/* ─── quicpro_set_client_cert ────────────────────────────────────────── */
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

/* ─── Function table ──────────────────────────────────────────────────── */
const zend_function_entry quicpro_tls_functions[] = {
    PHP_FE(quicpro_export_session_ticket, arginfo_quicpro_export_session_ticket)
    PHP_FE(quicpro_import_session_ticket, arginfo_quicpro_import_session_ticket)
    PHP_FE(quicpro_get_stats,             arginfo_quicpro_get_stats)
    PHP_FE(quicpro_set_ca_file,           arginfo_quicpro_set_ca_file)
    PHP_FE(quicpro_set_client_cert,       arginfo_quicpro_set_client_cert)
    PHP_FE_END
};
