/*
 * =========================================================================
 * FILENAME:   src/config.c
 * MODULE:     quicpro_async: Master Configuration Implementation
 * =========================================================================
 *
 * PURPOSE:
 * This file implements the C-level logic for the `Quicpro\Config` object.
 * It acts as the dispatcher for all session-level configuration,
 * translating a flexible PHP associative array into the rigid, high-
 * performance C structures required by the underlying `quiche` library and
 * the extension's own native modules.
 *
 * ARCHITECTURAL PRINCIPLES:
 *
 * 1.  Hybrid & Explicit Configuration: The framework uses a two-level
 * configuration model. System administrators can define global, baseline
 * defaults for all applications in `php.ini`. Developers then provide a
 * `Quicpro\Config` object for every `Session` or `Server` to override
 * these defaults on a per-session basis.
 *
 * 2.  Policy Enforcement: The ability for a developer to override `php.ini`
 * settings is itself a policy. The `quicpro.security_allow_config_override` INI
 * directive acts as a master switch. If set to `0` (the default), any attempt to pass
 * options to `Config::new()` will result in a `PolicyViolationException`.
 *
 * 3.  Immutability by Default ("Freezing"): Once a `Config` object is used
 * to create a `Session` or `Server`, it is marked as "frozen" and cannot
 * be mutated. This ensures consistent, thread-safe behavior in the
 * multi-worker environments enabled by `Quicpro\Cluster`.
 *
 * 4.  Live API Hot-Reloads: For zero-downtime reconfiguration, a running
 * server can have its active configuration updated via a secure MCP-based
 * Admin API. This is achieved via an atomic swap of config pointers.
 *
 * =========================================================================
 */

#include "php_quicpro.h"
#include "config.h"
#include "quicpro_ini.h"

#include <quiche.h>
#include <zend_smart_str.h>
#include <string.h>

// Forward declarations for internal dispatcher functions.
static void quicpro_config_init_all_modules(quicpro_cfg_t *cfg);
static void quicpro_config_apply_defaults(quicpro_cfg_t *cfg);
static void quicpro_config_apply_ini_settings(quicpro_cfg_t *cfg);
static void quicpro_config_apply_php_options(quicpro_cfg_t *cfg, HashTable *ht_opts);
static void quicpro_config_finalize_and_build(quicpro_cfg_t *cfg);

int le_quicpro_cfg;

static void quicpro_config_resource_dtor(zend_resource *rsrc)
{
    quicpro_cfg_t *cfg = (quicpro_cfg_t *) rsrc->ptr;
    if (cfg) {
        quicpro_config_free(cfg);
    }
}

void quicpro_config_free(quicpro_cfg_t *cfg)
{
    if (!cfg) return;

    if (cfg->q_cfg) {
        quiche_config_free(cfg->q_cfg);
    }

    quicpro_config_app_protocols_dtor(&cfg->app_protocols);
    quicpro_config_bare_metal_dtor(&cfg->bare_metal);
    quicpro_config_autoscale_dtor(&cfg->autoscale);
    quicpro_config_cluster_dtor(&cfg->cluster);
    quicpro_config_admin_api_dtor(&cfg->admin_api);
    quicpro_config_compute_ai_dtor(&cfg->compute_ai);
    quicpro_config_serialization_dtor(&cfg->serialization);
    quicpro_config_mcp_dtor(&cfg->mcp);
    quicpro_config_cdn_dtor(&cfg->cdn);
    quicpro_config_storage_dtor(&cfg->storage);
    quicpro_config_observability_dtor(&cfg->observability);
    quicpro_config_quic_dtor(&cfg->quic);
    quicpro_config_router_dtor(&cfg->router);
    quicpro_config_security_dtor(&cfg->security);
    quicpro_config_smart_contract_dtor(&cfg->smart_contract);
    quicpro_config_dns_dtor(&cfg->dns);
    quicpro_config_ssh_dtor(&cfg->ssh);
    quicpro_config_state_dtor(&cfg->state);
    quicpro_config_tcp_dtor(&cfg->tcp);
    quicpro_config_tls_dtor(&cfg->tls);

    efree(cfg);
}

quicpro_cfg_t* quicpro_config_new_from_options(zval *zopts)
{
    quicpro_cfg_t *cfg = ecalloc(1, sizeof(*cfg));
    cfg->q_cfg = quiche_config_new(QUICHE_PROTOCOL_VERSION);
    cfg->frozen = 0;

    quicpro_config_init_all_modules(cfg);
    quicpro_config_apply_defaults(cfg);
    quicpro_config_apply_ini_settings(cfg);

    if (zopts && Z_TYPE_P(zopts) == IS_ARRAY) {
        // The security config is applied first to get the override policy.
        quicpro_config_security_apply_ini(&cfg->security);
        quicpro_config_security_parse_options(&cfg->security, Z_ARRVAL_P(zopts));

        if (!cfg->security.allow_config_override && zend_hash_num_elements(Z_ARRVAL_P(zopts)) > 0) {
            quicpro_config_free(cfg);
            zend_throw_exception(quicpro_ce_policy_violation_exception, "Configuration override is disabled by system policy.", 0);
            return NULL;
        }
        quicpro_config_apply_php_options(cfg, Z_ARRVAL_P(zopts));
    }

    quicpro_config_finalize_and_build(cfg);
    return cfg;
}

PHP_FUNCTION(quicpro_new_config)
{
    zval *zopts = NULL;

    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY_OR_NULL(zopts)
    ZEND_PARSE_PARAMETERS_END();

    quicpro_cfg_t *cfg = quicpro_config_new_from_options(zopts);

    if (!cfg) {
        RETURN_THROWS();
    }

    RETURN_RES(zend_register_resource(cfg, le_quicpro_cfg));
}

void quicpro_config_mark_frozen(quicpro_cfg_t *cfg)
{
    if (cfg) {
        cfg->frozen = 1;
    }
}

static void quicpro_config_init_all_modules(quicpro_cfg_t *cfg)
{
    quicpro_config_app_protocols_init(&cfg->app_protocols);
    quicpro_config_bare_metal_init(&cfg->bare_metal);
    quicpro_config_autoscale_init(&cfg->autoscale);
    quicpro_config_cluster_init(&cfg->cluster);
    quicpro_config_admin_api_init(&cfg->admin_api);
    quicpro_config_compute_ai_init(&cfg->compute_ai);
    quicpro_config_serialization_init(&cfg->serialization);
    quicpro_config_mcp_init(&cfg->mcp);
    quicpro_config_cdn_init(&cfg->cdn);
    quicpro_config_storage_init(&cfg->storage);
    quicpro_config_observability_init(&cfg->observability);
    quicpro_config_quic_init(&cfg->quic);
    quicpro_config_router_init(&cfg->router);
    quicpro_config_security_init(&cfg->security);
    quicpro_config_smart_contract_init(&cfg->smart_contract);
    quicpro_config_dns_init(&cfg->dns);
    quicpro_config_ssh_init(&cfg->ssh);
    quicpro_config_state_init(&cfg->state);
    quicpro_config_tcp_init(&cfg->tcp);
    quicpro_config_tls_init(&cfg->tls);
}

static void quicpro_config_apply_defaults(quicpro_cfg_t *cfg)
{
    quicpro_config_app_protocols_apply_defaults(&cfg->app_protocols);
    quicpro_config_bare_metal_apply_defaults(&cfg->bare_metal);
    quicpro_config_autoscale_apply_defaults(&cfg->autoscale);
    quicpro_config_cluster_apply_defaults(&cfg->cluster);
    quicpro_config_admin_api_apply_defaults(&cfg->admin_api);
    quicpro_config_compute_ai_apply_defaults(&cfg->compute_ai);
    quicpro_config_serialization_apply_defaults(&cfg->serialization);
    quicpro_config_mcp_apply_defaults(&cfg->mcp);
    quicpro_config_cdn_apply_defaults(&cfg->cdn);
    quicpro_config_storage_apply_defaults(&cfg->storage);
    quicpro_config_observability_apply_defaults(&cfg->observability);
    quicpro_config_quic_apply_defaults(&cfg->quic);
    quicpro_config_router_apply_defaults(&cfg->router);
    quicpro_config_security_apply_defaults(&cfg->security);
    quicpro_config_smart_contract_apply_defaults(&cfg->smart_contract);
    quicpro_config_dns_apply_defaults(&cfg->dns);
    quicpro_config_ssh_apply_defaults(&cfg->ssh);
    quicpro_config_state_apply_defaults(&cfg->state);
    quicpro_config_tcp_apply_defaults(&cfg->tcp);
    quicpro_config_tls_apply_defaults(&cfg->tls);
}

static void quicpro_config_apply_ini_settings(quicpro_cfg_t *cfg)
{
    quicpro_config_app_protocols_apply_ini(&cfg->app_protocols);
    quicpro_config_bare_metal_apply_ini(&cfg->bare_metal);
    quicpro_config_autoscale_apply_ini(&cfg->autoscale);
    quicpro_config_cluster_apply_ini(&cfg->cluster);
    quicpro_config_admin_api_apply_ini(&cfg->admin_api);
    quicpro_config_compute_ai_apply_ini(&cfg->compute_ai);
    quicpro_config_serialization_apply_ini(&cfg->serialization);
    quicpro_config_mcp_apply_ini(&cfg->mcp);
    quicpro_config_cdn_apply_ini(&cfg->cdn);
    quicpro_config_storage_apply_ini(&cfg->storage);
    quicpro_config_observability_apply_ini(&cfg->observability);
    quicpro_config_quic_apply_ini(&cfg->quic);
    quicpro_config_router_apply_ini(&cfg->router);
    quicpro_config_security_apply_ini(&cfg->security);
    quicpro_config_smart_contract_apply_ini(&cfg->smart_contract);
    quicpro_config_dns_apply_ini(&cfg->dns);
    quicpro_config_ssh_apply_ini(&cfg->ssh);
    quicpro_config_state_apply_ini(&cfg->state);
    quicpro_config_tcp_apply_ini(&cfg->tcp);
    quicpro_config_tls_apply_ini(&cfg->tls);
}

static void quicpro_config_apply_php_options(quicpro_cfg_t *cfg, HashTable *ht_opts)
{
    quicpro_config_app_protocols_parse_options(&cfg->app_protocols, ht_opts);
    quicpro_config_bare_metal_parse_options(&cfg->bare_metal, ht_opts);
    quicpro_config_autoscale_parse_options(&cfg->autoscale, ht_opts);
    quicpro_config_cluster_parse_options(&cfg->cluster, ht_opts);
    quicpro_config_admin_api_parse_options(&cfg->admin_api, ht_opts);
    quicpro_config_compute_ai_parse_options(&cfg->compute_ai, ht_opts);
    quicpro_config_serialization_parse_options(&cfg->serialization, ht_opts);
    quicpro_config_mcp_parse_options(&cfg->mcp, ht_opts);
    quicpro_config_cdn_parse_options(&cfg->cdn, ht_opts);
    quicpro_config_storage_parse_options(&cfg->storage, ht_opts);
    quicpro_config_observability_parse_options(&cfg->observability, ht_opts);
    quicpro_config_quic_parse_options(&cfg->quic, ht_opts);
    quicpro_config_router_parse_options(&cfg->router, ht_opts);
    quicpro_config_smart_contract_parse_options(&cfg->smart_contract, ht_opts);
    quicpro_config_dns_parse_options(&cfg->dns, ht_opts);
    quicpro_config_ssh_parse_options(&cfg->ssh, ht_opts);
    quicpro_config_state_parse_options(&cfg->state, ht_opts);
    quicpro_config_tcp_parse_options(&cfg->tcp, ht_opts);
    quicpro_config_tls_parse_options(&cfg->tls, ht_opts);
}

static void quicpro_config_finalize_and_build(quicpro_cfg_t *cfg)
{
    // Only modules that directly map to the underlying quiche_config object
    // need a finalize function to apply their settings.
    quicpro_config_tls_finalize(&cfg->tls, cfg->q_cfg);
    quicpro_config_quic_finalize(&cfg->quic, cfg->q_cfg);
    quicpro_config_app_protocols_finalize(&cfg->app_protocols, cfg->q_cfg);
}

void quicpro_register_config_resource(int module_number)
{
    le_quicpro_cfg = zend_register_list_destructors_ex(
        quicpro_config_resource_dtor, NULL, "quicpro_config", module_number
    );
}