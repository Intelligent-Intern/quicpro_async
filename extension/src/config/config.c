/*
 * =========================================================================
 * FILENAME:   src/config.c
 * MODULE:     quicpro_async: Session Configuration Implementation
 * =========================================================================
 *
 * PURPOSE:
 * This file implements the C-level logic for the `Quicpro\Config` object.
 * It acts as the gatekeeper and parser for all session-level configuration,
 * translating a flexible PHP associative array into the rigid, high-
 * performance C structures required by the underlying `quiche` library and
 * the extension's own native modules.
 *
 * ARCHITECTURAL PRINCIPLES:
 *
 * 1.  Hybrid & Explicit Configuration: The framework uses a two-level
 * configuration model. System administrators can define global, baseline
 * defaults for all applications in `php.ini`. Developers then provide a
 * `Quicpro\Config` object for every `Session` or `Server`. This object can
 * either be empty to accept the global defaults, or it can contain specific
 * options to override them on a per-session basis. This makes all
 * dependencies explicit and behavior predictable.
 *
 * 2.  Policy Enforcement: The ability for a developer to override `php.ini`
 * settings is itself a policy. The `quicpro.allow_config_override` INI
 * directive acts as a master switch. If set to `0`, any attempt to pass
 * options to `Config::new()` will result in a `PolicyViolationException`.
 *
 * 3.  Immutability by Default ("Freezing"): Once a `Config` object is used
 * to create a `Session` or `Server`, it is marked as "frozen" and cannot
 * be mutated. This ensures consistent, thread-safe behavior in the
 * multi-worker environments enabled by `Quicpro\Cluster`. This also applies
 * to default configs created implicitly when a developer omits the config
 * argument.
 *
 * =========================================================================
 */

#include "php_quicpro.h"
#include "config.h"
#include "cors.h"
#include "quicpro_ini.h"
#include <quiche.h>
#include <openssl/ssl.h>
#include <zend_smart_str.h>
#include <string.h>

// Forward declarations for internal static helper functions.
static void apply_php_opts(quicpro_cfg_t *wr, HashTable *ht_opts);
static void apply_cors_opts(quicpro_cors_config_t *cors_cfg, zval *zv);

/**
 * The resource type identifier for `quicpro_cfg_t` objects, registered
 * with the Zend Engine during module startup.
 */
int le_quicpro_cfg;

/**
 * Resource Destructor for `quicpro_cfg_t`.
 *
 * This function is registered as a callback with the Zend Engine and is
 * automatically invoked when the reference count of a `Config` resource
 * drops to zero. Its crucial role is to release all underlying native
 * C-level resources to prevent memory leaks in long-running servers.
 *
 * @param rsrc A pointer to the Zend resource being destroyed.
 */
static void quicpro_cfg_dtor(zend_resource *rsrc)
{
    quicpro_cfg_t *wr = (quicpro_cfg_t *) rsrc->ptr;
    if (wr) {
        if (wr->cfg) {
            quiche_config_free(wr->cfg);
        }
        quicpro_cors_config_dtor(&wr->cors);
        efree(wr);
    }
}

/**
 * The C implementation for the PHP function `Quicpro\Config::new()`.
 *
 * This is the primary user-facing entry point for creating a session
 * configuration object. It applies settings in a clear order of precedence:
 * 1. Hardcoded safe defaults are applied first.
 * 2. Global `php.ini` settings are applied, overriding the hardcoded defaults.
 * 3. The user-provided `$options` array is applied last, overriding all previous values.
 */
PHP_FUNCTION(quicpro_new_config)
{
    zval *zopts = NULL;

    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY(zopts)
    ZEND_PARSE_PARAMETERS_END();

    quicpro_cfg_t *wr = ecalloc(1, sizeof(*wr));
    wr->cfg    = quiche_config_new(QUICHE_PROTOCOL_VERSION);
    wr->frozen = 0;
    quicpro_cors_config_init(&wr->cors);

    // Apply hardcoded, "75% performance" safe defaults.
    quiche_config_set_application_protos(wr->cfg, (const uint8_t*)"\x05h3", 3);
    quiche_config_set_max_idle_timeout(wr->cfg, 30000);
    quiche_config_set_max_send_udp_payload_size(wr->cfg, 1350);
    quiche_config_verify_peer(wr->cfg, true);

    // Apply global php.ini settings as a baseline override.
    if (qp_ini_cors_allowed_origins && strlen(qp_ini_cors_allowed_origins) > 0) {
        zval ini_val;
        ZVAL_STRING(&ini_val, qp_ini_cors_allowed_origins);
        apply_cors_options(&wr->cors, &ini_val);
        zval_ptr_dtor(&ini_val);
    }

    // Apply user-provided options from PHP, which have the highest precedence.
    if (zopts) {
        if (!qp_ini_allow_config_override && zend_hash_num_elements(Z_ARRVAL_P(zopts)) > 0) {
            zend_throw_exception(quicpro_ce_policy_violation_exception, "Configuration override is disabled by 'quicpro.allow_config_override' php.ini setting.", 0);
            efree(wr);
            RETURN_THROWS();
        }
        apply_php_opts(wr, Z_ARRVAL_P(zopts));
    }

    RETURN_RES(zend_register_resource(wr, le_quicpro_cfg));
}

/**
 * Internal helper to parse the `cors_allowed_origins` option from a PHP zval.
 * Encapsulates the logic for handling `false`, `'*'`, a comma-separated string,
 * or a PHP array of origin strings.
 */
static void apply_cors_options(quicpro_cors_config_t *cors_cfg, zval *zv)
{
    quicpro_cors_config_dtor(cors_cfg); // Clear any previously set config.

    if (Z_TYPE_P(zv) == IS_FALSE) {
        cors_cfg->enabled = false;
        return;
    }

    if (Z_TYPE_P(zv) == IS_STRING) {
        if (strcmp(Z_STRVAL_P(zv), "*") == 0) {
            cors_cfg->enabled = true;
            cors_cfg->allow_all_origins = true;
        } else {
            char *origins_str = estrndup(Z_STRVAL_P(zv), Z_STRLEN_P(zv));
            char *token = strtok(origins_str, ", ");
            while (token != NULL) {
                cors_cfg->num_allowed_origins++;
                cors_cfg->allowed_origins = erealloc(cors_cfg->allowed_origins, cors_cfg->num_allowed_origins * sizeof(char *));
                cors_cfg->allowed_origins[cors_cfg->num_allowed_origins - 1] = estrdup(token);
                token = strtok(NULL, ", ");
            }
            efree(origins_str);
            if (cors_cfg->num_allowed_origins > 0) cors_cfg->enabled = true;
        }
        return;
    }

    if (Z_TYPE_P(zv) == IS_ARRAY) {
        HashTable *ht_origins = Z_ARRVAL_P(zv);
        zend_long num_elements = zend_hash_num_elements(ht_origins);
        if (num_elements == 0) return;
        cors_cfg->allowed_origins = safe_emalloc(num_elements, sizeof(char *), 0);

        zval *origin_val;
        int i = 0;
        ZEND_HASH_FOREACH_VAL(ht_origins, origin_val) {
            if (Z_TYPE_P(origin_val) == IS_STRING) {
                cors_cfg->allowed_origins[i++] = estrndup(Z_STRVAL_P(origin_val), Z_STRLEN_P(origin_val));
            }
        } ZEND_HASH_FOREACH_END();
        cors_cfg->num_allowed_origins = i;
        if (cors_cfg->num_allowed_origins > 0) cors_cfg->enabled = true;
        return;
    }
}

/**
 * Main parser for the `$options` array passed to `Config::new()`.
 * Iterates through the HashTable and applies all recognized settings.
 */
static void apply_php_opts(quicpro_cfg_t *wr, HashTable *ht_opts)
{
    zval *zv;

    if ((zv = zend_hash_str_find(ht_opts, "cors_allowed_origins", sizeof("cors_allowed_origins")-1))) {
        apply_cors_options(&wr->cors, zv);
    }
    if ((zv = zend_hash_str_find(ht_opts, "verify_peer", sizeof("verify_peer")-1)) && zend_is_true(zv) == false) {
        quiche_config_verify_peer(wr->cfg, false);
    }
    if ((zv = zend_hash_str_find(ht_opts, "max_idle_timeout", sizeof("max_idle_timeout")-1)) && Z_TYPE_P(zv) == IS_LONG) {
        quiche_config_set_max_idle_timeout(wr->cfg, Z_LVAL_P(zv));
    }
    if ((zv = zend_hash_str_find(ht_opts, "max_pkt_size", sizeof("max_pkt_size")-1)) && Z_TYPE_P(zv) == IS_LONG) {
        quiche_config_set_max_send_udp_payload_size(wr->cfg, Z_LVAL_P(zv));
    }
    if ((zv = zend_hash_str_find(ht_opts, "alpn", sizeof("alpn")-1)) && Z_TYPE_P(zv) == IS_ARRAY) {
        smart_str alpn = {0};
        zval *val;
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(zv), val) {
            if (Z_TYPE_P(val) == IS_STRING && Z_STRLEN_P(val) < 256) {
                smart_str_appendc(&alpn, (unsigned char)Z_STRLEN_P(val));
                smart_str_appendl(&alpn, Z_STRVAL_P(val), Z_STRLEN_P(val));
            }
        } ZEND_HASH_FOREACH_END();
        if (alpn.s) {
            quiche_config_set_application_protos(wr->cfg, (uint8_t*)ZSTR_VAL(alpn.s), ZSTR_LEN(alpn.s));
            smart_str_free(&alpn);
        }
    }
    zval *z_cert = zend_hash_str_find(ht_opts, "cert_file", sizeof("cert_file")-1);
    zval *z_key  = zend_hash_str_find(ht_opts, "key_file", sizeof("key_file")-1);
    if (z_cert && z_key && Z_TYPE_P(z_cert) == IS_STRING && Z_TYPE_P(z_key) == IS_STRING) {
        quiche_config_load_cert_chain_from_pem_file(wr->cfg, Z_STRVAL_P(z_cert));
        quiche_config_load_priv_key_from_pem_file(wr->cfg, Z_STRVAL_P(z_key));
    }
    if ((zv = zend_hash_str_find(ht_opts, "ca_file", sizeof("ca_file")-1)) && Z_TYPE_P(zv) == IS_STRING) {
        quiche_config_load_verify_locations_from_file(wr->cfg, Z_STRVAL_P(zv));
    }
}

/**
 * These public-facing PHP functions allow for a procedural, step-by-step
 * configuration of a Config object before it is frozen.
 */
PHP_FUNCTION(quicpro_config_set_ca_file)
{
    zval *zcfg; char *file; size_t len;
    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_RESOURCE(zcfg)
        Z_PARAM_STRING(file, len)
    ZEND_PARSE_PARAMETERS_END();
    quicpro_cfg_t *wr = qp_fetch_cfg(zcfg);
    if (!wr || wr->frozen) RETURN_FALSE;
    if (quiche_config_load_verify_locations_from_file(wr->cfg, file) != 0) RETURN_FALSE;
    RETURN_TRUE;
}

PHP_FUNCTION(quicpro_config_set_client_cert)
{
    zval *zcfg; char *cert, *key; size_t lcert, lkey;
    ZEND_PARSE_PARAMETERS_START(3, 3)
        Z_PARAM_RESOURCE(zcfg)
        Z_PARAM_STRING(cert, lcert)
        Z_PARAM_STRING(key, lkey)
    ZEND_PARSE_PARAMETERS_END();
    quicpro_cfg_t *wr = qp_fetch_cfg(zcfg);
    if (!wr || wr->frozen) RETURN_FALSE;
    if (quiche_config_load_cert_chain_from_pem_file(wr->cfg, cert) != 0 ||
        quiche_config_load_priv_key_from_pem_file(wr->cfg, key)  != 0) {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}

PHP_FUNCTION(quicpro_config_export)
{
    zval *zcfg;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_RESOURCE(zcfg)
    ZEND_PARSE_PARAMETERS_END();
    quicpro_cfg_t *wr = qp_fetch_cfg(zcfg);
    if (!wr) RETURN_NULL();
    array_init(return_value);
    add_assoc_bool(return_value, "frozen", wr->frozen);
}

void quicpro_cfg_mark_frozen(zval *zcfg)
{
    quicpro_cfg_t *wr = qp_fetch_cfg(zcfg);
    if (wr) {
        wr->frozen = 1;
    }
}