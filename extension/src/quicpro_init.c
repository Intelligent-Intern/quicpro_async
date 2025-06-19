/*
 * =========================================================================
 * FILENAME:   src/quicpro_init.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Don't Panic.
 *
 * PURPOSE:
 * This file implements the master dispatcher for the registration of all
 * `php.ini` configuration settings within the `quicpro_async` extension,
 * as declared in `include/quicpro_init.h`.
 *
 * ARCHITECTURE:
 * This file contains the implementation of the functions defined in the
 * corresponding header. It serves as the central nervous system for the
 * extension's configuration lifecycle, called by the Zend Engine during the
 * `MINIT` and `MSHUTDOWN` phases to orchestrate the registration and
 * unregistration of INI directives from all logical sub-modules.
 *
 * THIS FILE DOES NOT INITIALIZE FUNCTIONAL LOGIC. It only registers the
 * configuration handlers. The actual implementation modules (e.g., the
 * cluster manager, the TLS engine) are initialized separately and will
 * subsequently read the configuration values that are loaded here.
 *
 * AUDIT & REVIEW NOTES:
 * This file is the entry point for understanding how the extension's
 * configuration is loaded. The order of `_init()` calls is critical.
 * The shutdown sequence is the exact reverse of initialization, which is
 * a standard and mandatory practice for unregistering INI handlers.
 * =========================================================================
 */

#include "include/quicpro_init.h"

/*
 * =========================================================================
 * == Modular Configuration Dependencies
 * =========================================================================
 * Each #include below represents a self-contained configuration module.
 * The corresponding `_init()` function registers the php.ini directives
 * for that specific domain.
 */
#include "include/config/app_http3_websockets_webtransport/index.h"
#include "include/config/bare_metal_tuning/index.h"
#include "include/config/cloud_autoscale/index.h"
#include "include/config/cluster_and_process/index.h"
#include "include/config/dynamic_admin_api/index.h"
#include "include/config/high_perf_compute_and_ai/index.h"
#include "include/config/iibin/index.h"
#include "include/config/mcp_and_orchestrator/index.h"
#include "include/config/native_cdn/index.h"
#include "include/config/native_object_store/index.h"
#include "include/config/open_telemetry/index.h"
#include "include/config/quic_transport/index.h"
#include "include/config/router_and_loadbalancer/index.h"
#include "include/config/security_and_traffic/index.h"
#include "include/config/smart_contracts/index.h"
#include "include/config/smart_dns/index.h"
#include "include/config/ssh_over_quic/index.h"
#include "include/config/state_management/index.h"
#include "include/config/tcp_transport/index.h"
#include "include/config/tls_and_crypto/index.h"

/*
 * =========================================================================
 * PHP_MINIT_FUNCTION Dispatcher
 * =========================================================================
 */
/**
 * @brief Master dispatcher for registering all module configurations.
 * @details This function is called once from `PHP_MINIT_FUNCTION`. It
 * orchestrates the registration of all `php.ini` directives by calling the
 * `_init()` function from each configuration module.
 *
 * @param type The type of initialization (e.g., MODULE_PERSISTENT).
 * @param module_number The unique number assigned to this extension by Zend.
 * @return `SUCCESS` on successful registration, `FAILURE` otherwise.
 */
int quicpro_init_modules(int type, int module_number)
{
    /*
     * The registration order is critical. We proceed from the most
     * fundamental, low-level modules to the highest-level application modules.
     */

    /*
     * Section 1: Foundational Layers (Process, Security, and Transport)
     * These modules' settings form the core of the extension. They must be
     * registered first. The Security module MUST come first to establish
     * the global override policy before any other module is handled.
     */
    qp_config_security_and_traffic_init();
    qp_config_cluster_and_process_init();
    qp_config_tls_and_crypto_init();
    qp_config_bare_metal_tuning_init();
    qp_config_quic_transport_init();
    qp_config_tcp_transport_init();

    /*
     * Section 2: Core Protocols & Data Handling
     * Register settings for modules that build directly on the transport layers.
     */
    qp_config_app_http3_websockets_webtransport_init();
    qp_config_mcp_and_orchestrator_init();
    qp_config_iibin_init();

    /*
     * Section 3: High-Level Services & Platform Features
     * Register settings for advanced, user-facing capabilities of the framework.
     */
    qp_config_state_management_init();
    qp_config_native_object_store_init();
    qp_config_high_perf_compute_and_ai_init();
    qp_config_cloud_autoscale_init();
    qp_config_open_telemetry_init();

    /*
     * Section 4: Specialized Operational Modes & Gateways
     * Register settings for highly specific server roles.
     */
    qp_config_native_cdn_init();
    qp_config_smart_dns_init();
    qp_config_ssh_over_quic_init();
    qp_config_router_and_loadbalancer_init();
    qp_config_smart_contracts_init();
    qp_config_dynamic_admin_api_init();

    return SUCCESS;
}

/*
 * =========================================================================
 * PHP_MSHUTDOWN_FUNCTION Dispatcher
 * =========================================================================
 */
/**
 * @brief Master dispatcher for unregistering all module configurations.
 * @details This function is called once from `PHP_MSHUTDOWN_FUNCTION`.
 * It orchestrates the clean shutdown of all configuration modules by
 * unregistering their INI entries.
 *
 * CRITICAL: The shutdown sequence MUST be the exact reverse of the
 * initialization sequence to prevent errors.
 *
 * @param type The type of shutdown (e.g., MODULE_PERSISTENT).
 * @param module_number The module number.
 * @return `SUCCESS` on successful shutdown, `FAILURE` otherwise.
 */
int quicpro_shutdown_modules(int type, int module_number)
{
    /* Unregister all configuration handlers in precise reverse order of registration. */

    /* Section 4: Specialized Operational Modes & Gateways */
    qp_config_dynamic_admin_api_shutdown();
    qp_config_smart_contracts_shutdown();
    qp_config_router_and_loadbalancer_shutdown();
    qp_config_ssh_over_quic_shutdown();
    qp_config_smart_dns_shutdown();
    qp_config_native_cdn_shutdown();

    /* Section 3: High-Level Services & Platform Features */
    qp_config_open_telemetry_shutdown();
    qp_config_cloud_autoscale_shutdown();
    qp_config_high_perf_compute_and_ai_shutdown();
    qp_config_native_object_store_shutdown();
    qp_config_state_management_shutdown();

    /* Section 2: Core Protocols & Data Handling */
    qp_config_iibin_shutdown();
    qp_config_mcp_and_orchestrator_shutdown();
    qp_config_app_http3_websockets_webtransport_shutdown();

    /* Section 1: Foundational Layers (Process, Security, and Transport) */
    qp_config_tcp_transport_shutdown();
    qp_config_quic_transport_shutdown();
    qp_config_bare_metal_tuning_shutdown();
    qp_config_tls_and_crypto_shutdown();
    qp_config_cluster_and_process_shutdown();
    qp_config_security_and_traffic_shutdown();

    return SUCCESS;
}

/*
 * =========================================================================
 * PHP_RINIT_FUNCTION Dispatcher
 * =========================================================================
 */
/**
 * @brief Master request initialization function.
 * @details Called from `PHP_RINIT_FUNCTION(quicpro_async)` at the beginning
 * of each PHP request. This hook is reserved for initializing
 * any per-request state or resources.
 *
 * NOTE: As of this version, no per-request initialization is needed
 * at the global level, but the hook is maintained for future use.
 */
int quicpro_request_init(int type, int module_number)
{
    return SUCCESS;
}

/*
 * =========================================================================
 * PHP_RSHUTDOWN_FUNCTION Dispatcher
 * =========================================================================
 */
/**
 * @brief Master request shutdown function.
 * @details Called from `PHP_RSHUTDOWN_FUNCTION(quicpro_async)` at the end of
 * each PHP request. This hook is reserved for cleaning up any
 * per-request state or resources to prevent memory leaks between
 * requests within the same process.
 *
 * NOTE: As of this version, no per-request cleanup is needed at the
 * global level.
 */
int quicpro_request_shutdown(int type, int module_number)
{
    return SUCCESS;
}
