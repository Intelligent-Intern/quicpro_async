/*
 * =========================================================================
 * FILENAME:   include/config/config.h
 * MODULE:     quicpro_async: Master Configuration System
 * =========================================================================
 *
 * OVERVIEW
 *
 * This header file defines the master configuration structure, `quicpro_cfg_t`,
 * which is exposed to PHP userland as the `Quicpro\Config` object. This
 * object acts as the single source of truth for all configurable parameters
 * within the entire `quicpro_async` framework.
 *
 * ARCHITECTURAL PRINCIPLES
 *
 * This system is built on a strict, 4-tier hierarchical configuration model.
 * Settings are applied in the following order, with each subsequent layer
 * overriding the previous one:
 *
 * TIER 1: C-LEVEL "SAFE DEFAULTS"
 * The absolute baseline. These are conservative, hardcoded values in the C
 * source code that prioritize stability and security over raw performance.
 *
 * TIER 2: GLOBAL `php.ini` OVERRIDES
 * A system administrator can establish a server-wide policy by setting
 * `quicpro.*` directives in `php.ini`.
 *
 * TIER 3: PER-SESSION `Quicpro\Config` OBJECT
 * A developer can instantiate `Quicpro\Config::new($options)` to override
 * the `php.ini` settings for a specific session. By default, this capability
 * is DISABLED for security (`quicpro.security_allow_config_override = 0`).
 *
 * TIER 4: LIVE ADMIN API HOT-RELOADS
 * For zero-downtime reconfiguration, a running server can have its active
 * configuration updated via a secure MCP-based Admin API.
 *
 * MODULAR STRUCTURE
 *
 * The C implementation is highly modular. This header file includes sub-headers
 * from each configuration domain (e.g., `tls_and_crypto/tls.h`). The master
 * `quicpro_cfg_t` struct is composed of smaller structs defined in these modules.
 *
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_H
#define QUICPRO_CONFIG_H

#include <php.h>
#include <quiche.h>

/*
 * =========================================================================
 * == Modular Configuration Includes
 * =========================================================================
 * Each header defines a `quicpro_cfg_[name]_t` struct for its own domain,
 * matching the new directory structure.
 */

#include "app_http3_websockets_webtransport/app_protocols.h"
#include "bare_metal_tuning/bare_metal.h"
#include "cloud_autoscale/autoscale.h"
#include "cluster_and_process/cluster.h"
#include "dynamic_admin_api/admin_api.h"
#include "high_perf_compute_and_ai/compute_ai.h"
#include "iibin/serialization.h"
#include "mcp_and_orchestrator/mcp.h"
#include "mcp_and_orchestrator/orchestrator.h"
#include "native_cdn/cdn.h"
#include "native_object_store/storage.h"
#include "open_telemetry/observability.h"
#include "quic_transport/quic.h"
#include "router_and_loadbalancer/router.h"
#include "security_and_traffic/security.h"
#include "smart_contracts/smart_contracts.h"
#include "smart_dns/dns.h"
#include "ssh_over_quic/ssh.h"
#include "state_management/state.h"
#include "tcp_transport/tcp.h"
#include "tls_and_crypto/tls.h"

/**
 * @brief The master C-level representation of a `Quicpro\Config` object.
 *
 * This struct is a "struct of structs", composed of the individual
 * configuration structures defined in the various sub-module headers.
 */
typedef struct quicpro_cfg_s {
    // The underlying raw config handle from the `quiche` library.
    quiche_config *quiche_cfg;

    // A flag that is set to true after the config is used, making it immutable.
    zend_bool frozen;

    // --- Composed Configuration Modules ---
    quicpro_cfg_app_protocols_t  app_protocols;
    quicpro_cfg_bare_metal_t   bare_metal;
    quicpro_cfg_autoscale_t    autoscale;
    quicpro_cfg_cluster_t      cluster;
    quicpro_cfg_admin_api_t    admin_api;
    quicpro_cfg_compute_ai_t   compute_ai;
    quicpro_cfg_serialization_t serialization;
    quicpro_cfg_mcp_t          mcp;
    quicpro_cfg_orchestrator_t orchestrator;
    quicpro_cfg_cdn_t          cdn;
    quicpro_cfg_storage_t      storage;
    quicpro_cfg_observability_t observability;
    quicpro_cfg_quic_t         quic;
    quicpro_cfg_router_t       router;
    quicpro_cfg_security_t     security;
    quicpro_cfg_smart_contract_t smart_contract;
    quicpro_cfg_dns_t          dns;
    quicpro_cfg_ssh_t          ssh;
    quicpro_cfg_state_t        state;
    quicpro_cfg_tcp_t          tcp;
    quicpro_cfg_tls_t          tls;

} quicpro_cfg_t;


/*
 * =========================================================================
 * == Public C-API for the Config Module
 * =========================================================================
 */

/**
 * @brief The main public PHP function to create a new configuration resource.
 *
 * PHP Signature: `Quicpro\Config::new(?array $options = null): object`
 */
PHP_FUNCTION(quicpro_new_config);

/**
 * @brief Creates a new `quicpro_cfg_t` struct from a PHP options array.
 *
 * This is the primary internal entry point. It allocates a new config struct
 * and applies the full 4-tier configuration hierarchy.
 *
 * @param zopts A zval pointing to a PHP associative array, or NULL.
 * @return A pointer to a newly allocated and fully populated `quicpro_cfg_t`.
 */
quicpro_cfg_t* quicpro_config_new_from_options(zval *zopts);

/**
 * @brief The resource destructor for a `quicpro_cfg_t`.
 *
 * Frees the `quicpro_cfg_t` struct itself and all underlying resources.
 *
 * @param cfg The configuration object to free.
 */
void quicpro_config_free(quicpro_cfg_t *cfg);

/**
 * @brief Marks a configuration object as immutable.
 *
 * @param cfg The configuration object to freeze.
 */
void quicpro_config_mark_frozen(quicpro_cfg_t *cfg);

#endif /* QUICPRO_CONFIG_H */