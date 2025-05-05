/*
 * config.c – quicpro configuration helpers
 * -----------------------------------------
 * This file defines the API for creating and managing reusable QUIC/TLS
 * configurations in PHP. By wrapping `quiche_config` in a Zend resource,
 * userland code can prepare connection parameters once and reuse them
 * across multiple sessions without re-parsing or reloading certificates.
 * Once a configuration resource is “frozen” by its first use, it becomes
 * immutable to prevent race conditions or inconsistent behavior.
 * The lifetime of the underlying `quiche_config` is tied to the PHP
 * garbage collector, so resources are cleaned up automatically when no
 * longer referenced.
 *
 * User‑land examples:
 *   $cfg = quicpro_new_config([
 *       'verify_peer'       => true,
 *       'ca_file'           => '/etc/ssl/certs/ca.pem',
 *       'cert_file'         => '/etc/ssl/certs/client.crt',
 *       'key_file'          => '/etc/ssl/private/client.key',
 *       'alpn'              => ['h3', 'hq-29'],
 *       'max_idle_timeout'  => 30000,
 *       'max_pkt_size'      => 1350,
 *   ]);
 *   quicpro_config_setCaFile($cfg, '/etc/ssl/certs/ca.pem');
 *   var_dump(quicpro_config_export($cfg));
 */

#include "php_quicpro.h"
#include <quiche.h>
#include <openssl/ssl.h>

/* -------------------------------------------------------------------------
 * Resource struct & Dtor
 * ------------------------------------------------------------------------- */

/*
 * quicpro_cfg_t holds the actual quiche_config pointer and a “frozen”
 * flag to prevent mutation after use. The `cfg` field points to the
 * underlying quiche configuration object, which stores TLS parameters,
 * ALPN settings, timeouts, and so on. The `frozen` boolean ensures that
 * once a configuration is bound to a live session, userland cannot alter
 * it, preserving consistency during the session’s lifetime. The
 * quicpro_cfg_dtor function is registered as the resource destructor and
 * is called by PHP’s garbage collector when the last reference to this
 * resource disappears. It safely frees the quiche_config, then nulls out
 * the pointer so double‐free errors cannot occur.
 */

typedef struct {
    quiche_config *cfg;    /* Underlying QUIC/TLS configuration handle */
    zend_bool      frozen; /* Marks whether the config has been used */
} quicpro_cfg_t;

static void quicpro_cfg_dtor(zend_resource *rsrc)
{
    quicpro_cfg_t *wr = (quicpro_cfg_t *) rsrc->ptr;
    if (wr && wr->cfg) {
        /* Release the memory allocated by quiche_config_new() */
        quiche_config_free(wr->cfg);
        wr->cfg = NULL;
    }
}

/*
 * The resource type `le_quicpro_cfg` must be registered in MINIT with
 * zend_register_list_destructors_ex, mapping this dtor function to the
 * “quicpro_cfg” name.  That tells PHP how to free the resource once
 * it is no longer referenced by any script.
 */

/* -------------------------------------------------------------------------
 * Helper – fetch & validate resource
 * ------------------------------------------------------------------------- */

/*
 * qp_fetch_cfg() is a convenience wrapper around zend_fetch_resource_ex().
 * It looks up a PHP zval (which should be a resource) and ensures it is of
 * the correct type (le_quicpro_cfg). If the resource does not exist or
 * is of the wrong type, PHP will emit an error and return NULL. This
 * helper centralizes that logic and casts the returned pointer to our
 * internal quicpro_cfg_t struct. All PHP‐facing functions that accept
 * a config resource call qp_fetch_cfg() before reading or modifying it.
 */

static inline quicpro_cfg_t *qp_fetch_cfg(zval *zcfg)
{
    return (quicpro_cfg_t *) zend_fetch_resource_ex(
        zcfg, "quicpro_cfg", le_quicpro_cfg
    );
}

/* -------------------------------------------------------------------------
 * INI‑style option mapping helpers
 * ------------------------------------------------------------------------- */

/*
 * qp_apply_ini_opts() applies user‐provided options from an associative
 * array (HashTable) onto the quiche_config. Each recognized key is
 * extracted from ht_opts, type‐checked, and applied via the quiche API.
 * Supported keys include `verify_peer`, toggling certificate verification;
 * `max_idle_timeout`, the number of milliseconds before the connection
 * is considered idle; `max_pkt_size`, controlling the UDP payload size;
 * and `alpn`, an array of protocol strings to advertise. In addition,
 * `cert_file`, `key_file`, and `ca_file` allow loading PEM files for
 * client certificates or CA bundles. Any unrecognized keys are silently
 * ignored, ensuring forward compatibility with future options.
 */

static void qp_apply_ini_opts(quicpro_cfg_t *wr, HashTable *ht_opts)
{
    zval *zv;

    /* Toggle peer verification on or off */
    if ((zv = zend_hash_str_find(ht_opts, "verify_peer", sizeof("verify_peer")-1))) {
        if (Z_TYPE_P(zv) == IS_FALSE) {
            quiche_config_verify_peer(wr->cfg, false);
        }
    }

    /* Override the default idle timeout if provided */
    if ((zv = zend_hash_str_find(ht_opts, "max_idle_timeout", sizeof("max_idle_timeout")-1))) {
        if (Z_TYPE_P(zv) == IS_LONG) {
            quiche_config_set_max_idle_timeout(wr->cfg, Z_LVAL_P(zv));
        }
    }

    /* Override the UDP payload size for sending packets */
    if ((zv = zend_hash_str_find(ht_opts, "max_pkt_size", sizeof("max_pkt_size")-1))) {
        if (Z_TYPE_P(zv) == IS_LONG) {
            quiche_config_set_max_send_udp_payload_size(wr->cfg, Z_LVAL_P(zv));
        }
    }

    /* Build the ALPN list: an array of strings to advertise protocols */
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

    /* Load an optional client cert & key for mutual-TLS */
    zval *z_cert = zend_hash_str_find(ht_opts, "cert_file", sizeof("cert_file")-1);
    zval *z_key  = zend_hash_str_find(ht_opts, "key_file",  sizeof("key_file")-1);
    if (z_cert && z_key &&
        Z_TYPE_P(z_cert) == IS_STRING && Z_TYPE_P(z_key) == IS_STRING
    ) {
        quiche_config_load_cert_chain_from_pem_file(wr->cfg, Z_STRVAL_P(z_cert));
        quiche_config_load_priv_key_from_pem_file   (wr->cfg, Z_STRVAL_P(z_key));
    }

    /* Load an alternate CA bundle if provided */
    if ((zv = zend_hash_str_find(ht_opts, "ca_file", sizeof("ca_file")-1))) {
        if (Z_TYPE_P(zv) == IS_STRING) {
            quiche_config_load_verify_locations_from_file(
                wr->cfg, Z_STRVAL_P(zv)
            );
        }
    }
}

/* -------------------------------------------------------------------------
 * PHP API: quicpro_new_config(array $opts)
 * ------------------------------------------------------------------------- */

/*
 * quicpro_new_config() allocates and initializes a new quiche_config
 * resource and registers it with PHP’s resource list. It accepts an
 * optional array of options, which it passes to qp_apply_ini_opts().
 * After allocation, it sets sane defaults for ALPN ("h3-29"), a 30 000 ms
 * idle timeout, and a 1350‐byte UDP payload size. If the user provided
 * an associative array, we apply those overrides. Finally, we return the
 * fresh resource back to PHP, ready for use by quicpro_connect().
 */

PHP_FUNCTION(quicpro_new_config)
{
    zval *zopts = NULL;

    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY(zopts)
    ZEND_PARSE_PARAMETERS_END();

    /* Allocate our wrapper and call quiche_config_new() */
    quicpro_cfg_t *wr = emalloc(sizeof(*wr));
    wr->cfg    = quiche_config_new(QUICHE_PROTOCOL_VERSION);
    wr->frozen = 0;

    /* Defaults: HTTP/3 ALPN, idle timeout, and UDP payload size */
    quiche_config_set_application_protos(
        wr->cfg, (const uint8_t*)"\x05h3-29", 6
    );
    quiche_config_set_max_idle_timeout(wr->cfg, 30000);
    quiche_config_set_max_send_udp_payload_size(wr->cfg, 1350);

    /* Apply any user‑provided overrides */
    if (zopts) {
        qp_apply_ini_opts(wr, Z_ARRVAL_P(zopts));
    }

    /* Register the resource so PHP can manage its lifecycle */
    RETURN_RES(zend_register_resource(wr, le_quicpro_cfg));
}

/* -------------------------------------------------------------------------
 * PHP API: quicpro_config_set_ca_file(resource $cfg, string $file)
 * ------------------------------------------------------------------------- */

/*
 * Allows dynamically setting the CA bundle path on an existing
 * non‐frozen config resource. We fetch and validate the resource,
 * verify that it has not yet been frozen by a connect(), then call
 * quiche_config_load_verify_locations_from_file(). Any failure is
 * reported by quicpro_set_error() and results in a FALSE return.
 * On success, returns TRUE, so userland can chain calls cleanly.
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
 * PHP API: quicpro_config_set_client_cert(resource $cfg, string $cert, string $key)
 * ------------------------------------------------------------------------- */

/*
 * Similar to set_ca_file(), this function allows loading a client
 * certificate and private key into a config resource before it is used.
 * We enforce that the resource is not frozen, then invoke the two
 * quiche library calls to load the PEM files. Any error triggers
 * quicpro_set_error() and a FALSE return; success yields TRUE.
 * This provides a simple PHP interface for mTLS setups.
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
 * PHP API: quicpro_config_export(resource $cfg): array
 * ------------------------------------------------------------------------- */

/*
 * Exports a minimal debug view of the config resource into a PHP array.
 * Currently it only reports the `frozen` flag, but could be extended
 * to include protocol version, timeout, ALPN list, etc. If the resource
 * is invalid, NULL is returned. Otherwise, an associative array is
 * initialized and populated with fields for inspection by userland
 * debugging or logging code.
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
 * Resource freeze hook – called from connect.c when cfg is first used
 * ------------------------------------------------------------------------- */

/*
 * Once a config resource is passed into quicpro_connect(), we mark it
 * as frozen so any further attempts to mutate it (via setCaFile or
 * setClientCert) will fail. This function is called automatically
 * by the connect implementation and simply sets the `frozen` flag.
 * Thereafter, the resource acts as a read‐only descriptor for quiche.
 */

void quicpro_cfg_mark_frozen(zval *zcfg)
{
    quicpro_cfg_t *wr = qp_fetch_cfg(zcfg);
    if (wr) {
        wr->frozen = 1;
    }
}
