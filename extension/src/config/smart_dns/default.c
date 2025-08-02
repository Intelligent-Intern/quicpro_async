/*
 * =========================================================================
 * FILENAME:   src/config/smart_dns/default.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    "Default settings are the speculations of yesterday that became
 * guidelines of today."
 *
 * PURPOSE:
 * Loads hard-coded, conservative default settings into the Smart-DNS
 * configuration struct.
 * =========================================================================
 */

#include "include/config/smart_dns/default.h"
#include "include/config/smart_dns/base_layer.h"
#include "php.h"

void qp_config_smart_dns_defaults_load(void)
{
    /* --- General Server Settings --- */
    quicpro_smart_dns_config.dns_server_enable = false;
    quicpro_smart_dns_config.dns_server_bind_host = pestrdup("0.0.0.0", 1);
    quicpro_smart_dns_config.dns_server_port = 53;
    quicpro_smart_dns_config.dns_server_enable_tcp = true;
    quicpro_smart_dns_config.dns_default_record_ttl_sec = 60;

    /* --- Operational Mode --- */
    quicpro_smart_dns_config.dns_mode = pestrdup("service_discovery", 1);
    quicpro_smart_dns_config.dns_static_zone_file_path = pestrdup("/etc/quicpro/dns/zones.db", 1);
    quicpro_smart_dns_config.dns_recursive_forwarders = pestrdup("", 1);

    /* Service-discovery mode specifics */
    quicpro_smart_dns_config.dns_health_agent_mcp_endpoint = pestrdup("127.0.0.1:9998", 1);
    quicpro_smart_dns_config.dns_service_discovery_max_ips_per_response = 8;

    /* Security & EDNS */
    quicpro_smart_dns_config.dns_enable_dnssec_validation = true;
    quicpro_smart_dns_config.dns_edns_udp_payload_size = 1232;

    /* Semantic DNS */
    quicpro_smart_dns_config.dns_semantic_mode_enable = false;
    quicpro_smart_dns_config.dns_mothernode_uri = pestrdup("", 1);
    quicpro_smart_dns_config.dns_mothernode_sync_interval_sec = 86400;
}
