/*
 * quicpro configuration helpers
 * -----------------------------------------
 * Provides a thin Zend resource wrapper around `quiche_config` so that
 * Userland can prepare re‑usable QUIC+TLS parameter sets **before** opening a
 * connection. The object is immutable after first use, but the pointer
 * remains valid until GC runs the resource dtor.
 *
 *  User‑land API (php):
 *  -------------------
 *      $cfg = quicpro_new_config([
 *          'verify_peer' => true,
 *          'ca_file'     => '/etc/ssl/certs/ca.pem',
 *          'cert_file'   => '/etc/ssl/certs/client.crt',
 *          'key_file'    => '/etc/ssl/private/client.key',
 *          'alpn'        => ['h3', 'hq-29'],
 *          'max_idle_timeout' => 30_000,
 *          'max_pkt_size' => 1350,
 *      ]);
 *
 *      quicpro_config_setCaFile($cfg, '/etc/ssl/certs/ca.pem');
 *      quicpro_config_export($cfg);   // → associative array for debug
 */

#include "php_quicpro.h"
#include <quiche.h>
#include <openssl/ssl.h>

/* -------------------------------------------------------------------------
 *  Resource struct & Dtor
 * ------------------------------------------------------------------------- */

typedef struct {
    quiche_config *cfg;
    zend_bool      frozen;   /* turned true when first connection uses it */
} quicpro_cfg_t;

static void quicpro_cfg_dtor(zend_resource *rsrc)
{
    quicpro_cfg_t *wr = (quicpro_cfg_t *) rsrc->ptr;
    if (wr && wr->cfg) {
        quiche_config_free(wr->cfg);
        wr->cfg = NULL;
    }
}

/* Register the resource handler at MINIT in your module header */
/* le_quicpro_cfg is declared in php_quicpro.h enum */

/* -------------------------------------------------------------------------
 *  Helper – fetch & validate resource
 * ------------------------------------------------------------------------- */
static inline quicpro_cfg_t *qp_fetch_cfg(zval *zcfg)
{
    return (quicpro_cfg_t *) zend_fetch_resource_ex(zcfg, "quicpro_cfg", le_quicpro_cfg);
}

/* -------------------------------------------------------------------------
 *  INI‑style option mapping helpers
 * ------------------------------------------------------------------------- */
static void qp_apply_ini_opts(quicpro_cfg_t *wr, HashTable *ht_opts)
{
    zval *zv;

    if ((zv = zend_hash_str_find(ht_opts, "verify_peer", sizeof("verify_peer")-1))) {
        if (Z_TYPE_P(zv) == IS_FALSE) {
            quiche_config_verify_peer(wr->cfg, false);
        }
    }

    if ((zv = zend_hash_str_find(ht_opts, "max_idle_timeout", sizeof("max_idle_timeout")-1))) {
        if (Z_TYPE_P(zv) == IS_LONG) {
            quiche_config_set_max_idle_timeout(wr->cfg, Z_LVAL_P(zv));
        }
    }

    if ((zv = zend_hash_str_find(ht_opts, "max_pkt_size", sizeof("max_pkt_size")-1))) {
        if (Z_TYPE_P(zv) == IS_LONG) {
            quiche_config_set_max_send_udp_payload_size(wr->cfg, Z_LVAL_P(zv));
        }
    }

    /* ALPN list – array of strings */
    if ((zv = zend_hash_str_find(ht_opts, "alpn", sizeof("alpn")-1))) {
        if (Z_TYPE_P(zv) == IS_ARRAY) {
            smart_str alpn = {0};
            zval *val;
            ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(zv), val) {
                if (Z_TYPE_P(val) == IS_STRING && Z_STRLEN_P(val) < 256) {
                    uint8_t len = (uint8_t) Z_STRLEN_P(val);
                    smart_str_appendc(&alpn, len);
                    smart_str_appendl(&alpn, Z_STRVAL_P(val), len);
                }
            } ZEND_HASH_FOREACH_END();
            smart_str_0(&alpn);
            if (alpn.s) {
                quiche_config_set_application_protos(wr->cfg,
                    (uint8_t*) ZSTR_VAL(alpn.s), ZSTR_LEN(alpn.s));
            }
            smart_str_free(&alpn);
        }
    }

    /* cert/key & ca file */
    zval *z_cert = zend_hash_str_find(ht_opts, "cert_file", sizeof("cert_file")-1);
    zval *z_key  = zend_hash_str_find(ht_opts, "key_file", sizeof("key_file")-1);
    if (z_cert && z_key && Z_TYPE_P(z_cert)==IS_STRING && Z_TYPE_P(z_key)==IS_STRING) {
        quiche_config_load_cert_chain_from_pem_file(wr->cfg, Z_STRVAL_P(z_cert));
        quiche_config_load_priv_key_from_pem_file(wr->cfg,  Z_STRVAL_P(z_key));
    }

    if ((zv = zend_hash_str_find(ht_opts, "ca_file", sizeof("ca_file")-1))) {
        if (Z_TYPE_P(zv) == IS_STRING) {
            quiche_config_load_verify_locations_from_file(wr->cfg, Z_STRVAL_P(zv));
        }
    }
}

/* -------------------------------------------------------------------------
 *  PHP API: quicpro_new_config(array $opts)
 * ------------------------------------------------------------------------- */
PHP_FUNCTION(quicpro_new_config)
{
    zval *zopts = NULL;

    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY(zopts)
    ZEND_PARSE_PARAMETERS_END();

    quicpro_cfg_t *wr = emalloc(sizeof(*wr));
    wr->cfg    = quiche_config_new(QUICHE_PROTOCOL_VERSION);
    wr->frozen = 0;

    /* apply defaults */
    quiche_config_set_application_protos(wr->cfg,(const uint8_t*)"\x05h3-29",6);
    quiche_config_set_max_idle_timeout(wr->cfg,30000);
    quiche_config_set_max_send_udp_payload_size(wr->cfg,1350);

    if (zopts) {
        qp_apply_ini_opts(wr, Z_ARRVAL_P(zopts));
    }

    RETURN_RES(zend_register_resource(wr, le_quicpro_cfg));
}

/* -------------------------------------------------------------------------
 *  PHP API: quicpro_config_setCaFile(resource $cfg, string $file)
 * ------------------------------------------------------------------------- */
PHP_FUNCTION(quicpro_config_set_ca_file)
{
    zval *zcfg;
    char *file;
    size_t len;

    ZEND_PARSE_PARAMETERS_START(2,2)
        Z_PARAM_RESOURCE(zcfg)
        Z_PARAM_STRING(file,len)
    ZEND_PARSE_PARAMETERS_END();

    quicpro_cfg_t *wr = qp_fetch_cfg(zcfg);
    if (!wr || wr->frozen) {
        RETURN_FALSE;
    }

    if (quiche_config_load_verify_locations_from_file(wr->cfg, file) != 0) {
        quicpro_set_error("Failed to load CA bundle");
        RETURN_FALSE;
    }
    RETURN_TRUE;
}

/* -------------------------------------------------------------------------
 *  PHP API: quicpro_config_set_client_cert(resource $cfg, string $cert, string $key)
 * ------------------------------------------------------------------------- */
PHP_FUNCTION(quicpro_config_set_client_cert)
{
    zval *zcfg;
    char *cert,*key;
    size_t lcert,lkey;

    ZEND_PARSE_PARAMETERS_START(3,3)
        Z_PARAM_RESOURCE(zcfg)
        Z_PARAM_STRING(cert,lcert)
        Z_PARAM_STRING(key,lkey)
    ZEND_PARSE_PARAMETERS_END();

    quicpro_cfg_t *wr = qp_fetch_cfg(zcfg);
    if (!wr || wr->frozen) {
        RETURN_FALSE;
    }

    if (quiche_config_load_cert_chain_from_pem_file(wr->cfg, cert) != 0 ||
        quiche_config_load_priv_key_from_pem_file(wr->cfg, key) != 0) {
        quicpro_set_error("Failed to load client cert or key");
        RETURN_FALSE;
    }
    RETURN_TRUE;
}

/* -------------------------------------------------------------------------
 *  PHP API: quicpro_config_export(resource $cfg): array
 * ------------------------------------------------------------------------- */
PHP_FUNCTION(quicpro_config_export)
{
    zval *zcfg;
    ZEND_PARSE_PARAMETERS_START(1,1)
        Z_PARAM_RESOURCE(zcfg)
    ZEND_PARSE_PARAMETERS_END();

    quicpro_cfg_t *wr = qp_fetch_cfg(zcfg);
    if (!wr) RETURN_NULL();

    array_init(return_value);
    add_assoc_bool(return_value, "frozen", wr->frozen);
    /* More fields can be exposed using quiche_config_get_* once available */
}

/* -------------------------------------------------------------------------
 *  Resource freeze hook – called from connect.c when cfg is first used
 * ------------------------------------------------------------------------- */
void quicpro_cfg_mark_frozen(zval *zcfg)
{
    quicpro_cfg_t *wr = qp_fetch_cfg(zcfg);
    if (wr) wr->frozen = 1;
}

#endif /* PHP_QUICPRO_CONFIG_C */
