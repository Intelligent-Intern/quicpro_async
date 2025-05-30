/*
 * config.c – quicpro configuration helpers
 * -----------------------------------------
 * Implements all APIs for creating, managing, and exporting reusable
 * QUIC/TLS configuration objects (quiche_config) as PHP resources.
 * Provides mutation before connect(), and a freeze mechanism
 * (once used in a session, config becomes read-only).
 * Fully garbage-collected via Zend resource list.
 *
 * See config.h for public types and prototypes.
 */

#include "php_quicpro.h"    /* Core extension header, resource IDs, error helpers */
#include "config.h"         /* Our config resource struct, prototypes */
#include <quiche.h>         /* quiche_config and API */
#include <openssl/ssl.h>    /* Only for some legacy compat – rarely needed */
#include <zend_smart_str.h> /* For building ALPN proto list */
#include <string.h>         /* For memset, etc. */

/* -------------------------------------------------------------------------
 * Resource Type ID (extern in .h, actually allocated here at module init)
 * -----------------------------------------------------------------------*/
int le_quicpro_cfg;

/* -------------------------------------------------------------------------
 * Resource struct & Dtor
 * -----------------------------------------------------------------------*/

/*
 * quicpro_cfg_dtor
 * ----------------
 * Zend resource destructor: safely releases underlying quiche_config and
 * zeroes pointer to avoid double-free. Called when last PHP ref dies.
 */
static void quicpro_cfg_dtor(zend_resource *rsrc)
{
    quicpro_cfg_t *wr = (quicpro_cfg_t *) rsrc->ptr;
    if (wr && wr->cfg) {
        quiche_config_free(wr->cfg);
        wr->cfg = NULL;
    }
}

/* -------------------------------------------------------------------------
 * PHP: quicpro_new_config([array $opts]) -> resource
 * -----------------------------------------------------------------------*/

/*
 * Allocates a new config resource. Sets sane HTTP/3 defaults.
 * Optionally applies user-provided settings (see qp_apply_ini_opts).
 * Returns a PHP resource to userland, GC-managed.
 */
PHP_FUNCTION(quicpro_new_config)
{
    zval *zopts = NULL;

    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY(zopts)
    ZEND_PARSE_PARAMETERS_END();

    /* Allocate wrapper and init quiche_config */
    quicpro_cfg_t *wr = emalloc(sizeof(*wr));
    wr->cfg    = quiche_config_new(QUICHE_PROTOCOL_VERSION);
    wr->frozen = 0;

    /* Defaults: ALPN "h3-29", idle timeout, UDP payload size */
    quiche_config_set_application_protos(
        wr->cfg, (const uint8_t*)"\x05h3-29", 6
    );
    quiche_config_set_max_idle_timeout(wr->cfg, 30000);
    quiche_config_set_max_send_udp_payload_size(wr->cfg, 1350);

    /* Optionally apply user overrides */
    if (zopts) {
        qp_apply_ini_opts(wr, Z_ARRVAL_P(zopts));
    }

    /* Register resource so PHP manages its lifetime */
    RETURN_RES(zend_register_resource(wr, le_quicpro_cfg));
}

/* -------------------------------------------------------------------------
 * Helper: Apply options from array onto quiche_config
 * -----------------------------------------------------------------------*/

/*
 * Applies keys from user $opts array onto the quiche_config.
 * Recognized options:
 *  - verify_peer (bool)
 *  - max_idle_timeout (int)
 *  - max_pkt_size (int)
 *  - alpn (array of string)
 *  - cert_file (string)
 *  - key_file (string)
 *  - ca_file (string)
 * Unrecognized keys are ignored for forward-compat.
 */
static void qp_apply_ini_opts(quicpro_cfg_t *wr, HashTable *ht_opts)
{
    zval *zv;

    /* verify_peer (off disables peer certificate verification) */
    if ((zv = zend_hash_str_find(ht_opts, "verify_peer", sizeof("verify_peer")-1))) {
        if (Z_TYPE_P(zv) == IS_FALSE) {
            quiche_config_verify_peer(wr->cfg, false);
        }
    }

    /* max_idle_timeout (milliseconds) */
    if ((zv = zend_hash_str_find(ht_opts, "max_idle_timeout", sizeof("max_idle_timeout")-1))) {
        if (Z_TYPE_P(zv) == IS_LONG) {
            quiche_config_set_max_idle_timeout(wr->cfg, Z_LVAL_P(zv));
        }
    }

    /* max_pkt_size (UDP datagram payload size) */
    if ((zv = zend_hash_str_find(ht_opts, "max_pkt_size", sizeof("max_pkt_size")-1))) {
        if (Z_TYPE_P(zv) == IS_LONG) {
            quiche_config_set_max_send_udp_payload_size(wr->cfg, Z_LVAL_P(zv));
        }
    }

    /* alpn (array of string – serialized into length-prefixed wire format) */
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
                quiche_config_set_application_protos(
                    wr->cfg,
                    (uint8_t*) ZSTR_VAL(alpn.s),
                    ZSTR_LEN(alpn.s)
                );
            }
            smart_str_free(&alpn);
        }
    }

    /* cert_file & key_file for mTLS */
    zval *z_cert = zend_hash_str_find(ht_opts, "cert_file", sizeof("cert_file")-1);
    zval *z_key  = zend_hash_str_find(ht_opts, "key_file",  sizeof("key_file")-1);
    if (z_cert && z_key &&
        Z_TYPE_P(z_cert) == IS_STRING && Z_TYPE_P(z_key) == IS_STRING
    ) {
        quiche_config_load_cert_chain_from_pem_file(wr->cfg, Z_STRVAL_P(z_cert));
        quiche_config_load_priv_key_from_pem_file   (wr->cfg, Z_STRVAL_P(z_key));
    }

    /* ca_file (trusted CA bundle for peer cert verification) */
    if ((zv = zend_hash_str_find(ht_opts, "ca_file", sizeof("ca_file")-1))) {
        if (Z_TYPE_P(zv) == IS_STRING) {
            quiche_config_load_verify_locations_from_file(
                wr->cfg, Z_STRVAL_P(zv)
            );
        }
    }
}

/* -------------------------------------------------------------------------
 * PHP: quicpro_config_set_ca_file(resource $cfg, string $file) -> bool
 * -----------------------------------------------------------------------*/

/*
 * Loads CA file into non-frozen config. Returns true on success, false on error.
 */
PHP_FUNCTION(quicpro_config_set_ca_file)
{
    zval *zcfg;
    char *file;
    size_t len;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_RESOURCE(zcfg)
        Z_PARAM_STRING(file, len)
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
 * PHP: quicpro_config_set_client_cert(resource $cfg, string $cert, string $key) -> bool
 * -----------------------------------------------------------------------*/

/*
 * Loads client cert & key into non-frozen config. Returns true on success, false on error.
 */
PHP_FUNCTION(quicpro_config_set_client_cert)
{
    zval *zcfg;
    char *cert, *key;
    size_t lcert, lkey;

    ZEND_PARSE_PARAMETERS_START(3, 3)
        Z_PARAM_RESOURCE(zcfg)
        Z_PARAM_STRING(cert, lcert)
        Z_PARAM_STRING(key,  lkey)
    ZEND_PARSE_PARAMETERS_END();

    quicpro_cfg_t *wr = qp_fetch_cfg(zcfg);
    if (!wr || wr->frozen) {
        RETURN_FALSE;
    }

    if (quiche_config_load_cert_chain_from_pem_file(wr->cfg, cert) != 0 ||
        quiche_config_load_priv_key_from_pem_file   (wr->cfg, key)  != 0
    ) {
        quicpro_set_error("Failed to load client cert or key");
        RETURN_FALSE;
    }
    RETURN_TRUE;
}

/* -------------------------------------------------------------------------
 * PHP: quicpro_config_export(resource $cfg) -> array|null
 * -----------------------------------------------------------------------*/

/*
 * Exports a minimal debug view of the config resource as PHP array.
 * Only 'frozen' is exported for now; extend as needed.
 */
PHP_FUNCTION(quicpro_config_export)
{
    zval *zcfg;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_RESOURCE(zcfg)
    ZEND_PARSE_PARAMETERS_END();

    quicpro_cfg_t *wr = qp_fetch_cfg(zcfg);
    if (!wr) {
        RETURN_NULL();
    }

    array_init(return_value);
    add_assoc_bool(return_value, "frozen", wr->frozen);
}

/* -------------------------------------------------------------------------
 * quicpro_cfg_mark_frozen(zval *zcfg)
 * -----------------------------------------------------------------------*/

/*
 * Called automatically after config used in connect().
 * Marks resource as immutable to prevent further changes.
 */
void quicpro_cfg_mark_frozen(zval *zcfg)
{
    quicpro_cfg_t *wr = qp_fetch_cfg(zcfg);
    if (wr) {
        wr->frozen = 1;
    }
}

/* -------------------------------------------------------------------------
 * Resource Registration (called in MINIT, not here)
 * -----------------------------------------------------------------------*/
/*
 * In your MINIT, register the config resource like so:
 * le_quicpro_cfg = zend_register_list_destructors_ex(
 *     quicpro_cfg_dtor, NULL, "quicpro_cfg", module_number);
 *
 * This ensures all config resources are GC-managed and their quiche_config
 * are freed when appropriate.
 */

