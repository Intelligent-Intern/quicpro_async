/*
 * =========================================================================
 * FILENAME:   include/config/smart_dns/base_layer.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    The universe is a pretty big place. It's bigger than anything
 * anyone has ever dreamed of before.
 *
 * PURPOSE:
 * This header file defines the core data structure for the `smart_dns`
 * configuration module.
 *
 * ARCHITECTURE:
 * This struct holds the configuration values for running the framework
 * as a high-performance, fully-featured DNS server.
 * =========================================================================
 */
#ifndef QUICPRO_CONFIG_SMART_DNS_BASE_H
#define QUICPRO_CONFIG_SMART_DNS_BASE_H

#include "php.h"
#include <stdbool.h>

typedef struct _qp_smart_dns_config_t {
    /* --- General Server Settings --- */
    bool server_enable;
    char *server_bind_host;
    zend_long server_port;
    bool server_enable_tcp;
    zend_long default_record_ttl_sec;

    /* --- Operational Mode --- */
    char *mode;
    char *static_zone_file_path;
    char *recursive_forwarders;

    /* --- Service Discovery Mode Settings --- */
    char *health_agent_mcp_endpoint;
    zend_long service_discovery_max_ips_per_response;

    /* --- Security & EDNS --- */
    bool enable_dnssec_validation;
    zend_long edns_udp_payload_size;

    /* --- Semantic DNS & Mothernode (Future Vision) --- */
    bool semantic_mode_enable;
    char *mothernode_uri;
    zend_long mothernode_sync_interval_sec;

} qp_smart_dns_config_t;

/* The single instance of this module's configuration data */
extern qp_smart_dns_config_t quicpro_smart_dns_config;

#endif /* QUICPRO_CONFIG_SMART_DNS_BASE_H */
