/*
 * =========================================================================
 * FILENAME:   src/config/smart_dns/ini.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    "Change your DNS settings carefully – the whole world relies on them."
 *
 * PURPOSE:
 * Registers, parses and validates all `php.ini` directives for the Smart‑DNS
 * configuration module.
 * =========================================================================
 */

#include "include/config/smart_dns/ini.h"
#include "include/config/smart_dns/base_layer.h"

#include "php.h"
#include <zend_ini.h>
#include <ext/spl/spl_exceptions.h>

/* Positive long validator */
static ZEND_INI_MH(OnUpdateDnsPositiveLong)
{
    zend_long val = zend_atol(ZSTR_VAL(new_value), ZSTR_LEN(new_value));

    if (val <= 0) {
        zend_throw_exception_ex(
            spl_ce_InvalidArgumentException, 0,
            "Invalid value provided for Smart‑DNS directive. A positive integer is required.");
        return FAILURE;
    }

    if (zend_string_equals_literal(entry->name, "quicpro.dns_server_port")) {
        quicpro_smart_dns_config.dns_server_port = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.dns_default_record_ttl_sec")) {
        quicpro_smart_dns_config.dns_default_record_ttl_sec = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.dns_service_discovery_max_ips_per_response")) {
        quicpro_smart_dns_config.dns_service_discovery_max_ips_per_response = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.dns_edns_udp_payload_size")) {
        quicpro_smart_dns_config.dns_edns_udp_payload_size = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.dns_mothernode_sync_interval_sec")) {
        quicpro_smart_dns_config.dns_mothernode_sync_interval_sec = val;
    }

    return SUCCESS;
}

/* Enumerated string validator for dns_mode */
static ZEND_INI_MH(OnUpdateDnsModeString)
{
    const char *allowed[] = {"authoritative", "recursive_resolver", "service_discovery", NULL};
    int i;
    for (i = 0; allowed[i]; ++i) {
        if (zend_string_equals_literal(new_value, allowed[i])) {
            quicpro_smart_dns_config.dns_mode = estrdup(ZSTR_VAL(new_value));
            return SUCCESS;
        }
    }

    zend_throw_exception_ex(
        spl_ce_InvalidArgumentException, 0,
        "Invalid dns_mode specified for Smart‑DNS module.");
    return FAILURE;
}

static ZEND_INI_MH(OnUpdateStringDuplicate)
{
    /* Delegates to Zend helper to duplicate persistent string */
    OnUpdateString(entry, new_value, mh_arg1, mh_arg2, mh_arg3);
    return SUCCESS;
}

PHP_INI_BEGIN()
    /* Master switch */
    STD_PHP_INI_ENTRY("quicpro.dns_server_enable", "0", PHP_INI_SYSTEM,
        OnUpdateBool, dns_server_enable, qp_smart_dns_config_t, quicpro_smart_dns_config)

    /* General */
    ZEND_INI_ENTRY_EX("quicpro.dns_server_bind_host", "0.0.0.0", PHP_INI_SYSTEM,
        OnUpdateStringDuplicate, &quicpro_smart_dns_config.dns_server_bind_host, NULL, NULL)

    ZEND_INI_ENTRY_EX("quicpro.dns_server_port", "53", PHP_INI_SYSTEM,
        OnUpdateDnsPositiveLong, NULL, NULL, NULL)

    STD_PHP_INI_ENTRY("quicpro.dns_server_enable_tcp", "1", PHP_INI_SYSTEM,
        OnUpdateBool, dns_server_enable_tcp, qp_smart_dns_config_t, quicpro_smart_dns_config)

    ZEND_INI_ENTRY_EX("quicpro.dns_default_record_ttl_sec", "60", PHP_INI_SYSTEM,
        OnUpdateDnsPositiveLong, NULL, NULL, NULL)

    /* Operational mode */
    ZEND_INI_ENTRY_EX("quicpro.dns_mode", "service_discovery", PHP_INI_SYSTEM,
        OnUpdateDnsModeString, NULL, NULL, NULL)

    ZEND_INI_ENTRY_EX("quicpro.dns_static_zone_file_path", "/etc/quicpro/dns/zones.db", PHP_INI_SYSTEM,
        OnUpdateStringDuplicate, &quicpro_smart_dns_config.dns_static_zone_file_path, NULL, NULL)

    ZEND_INI_ENTRY_EX("quicpro.dns_recursive_forwarders", "", PHP_INI_SYSTEM,
        OnUpdateStringDuplicate, &quicpro_smart_dns_config.dns_recursive_forwarders, NULL, NULL)

    ZEND_INI_ENTRY_EX("quicpro.dns_health_agent_mcp_endpoint", "127.0.0.1:9998", PHP_INI_SYSTEM,
        OnUpdateStringDuplicate, &quicpro_smart_dns_config.dns_health_agent_mcp_endpoint, NULL, NULL)

    ZEND_INI_ENTRY_EX("quicpro.dns_service_discovery_max_ips_per_response", "8", PHP_INI_SYSTEM,
        OnUpdateDnsPositiveLong, NULL, NULL, NULL)

    /* Security & EDNS */
    STD_PHP_INI_ENTRY("quicpro.dns_enable_dnssec_validation", "1", PHP_INI_SYSTEM,
        OnUpdateBool, dns_enable_dnssec_validation, qp_smart_dns_config_t, quicpro_smart_dns_config)

    ZEND_INI_ENTRY_EX("quicpro.dns_edns_udp_payload_size", "1232", PHP_INI_SYSTEM,
        OnUpdateDnsPositiveLong, NULL, NULL, NULL)

    /* Semantic DNS */
    STD_PHP_INI_ENTRY("quicpro.dns_semantic_mode_enable", "0", PHP_INI_SYSTEM,
        OnUpdateBool, dns_semantic_mode_enable, qp_smart_dns_config_t, quicpro_smart_dns_config)

    ZEND_INI_ENTRY_EX("quicpro.dns_mothernode_uri", "", PHP_INI_SYSTEM,
        OnUpdateStringDuplicate, &quicpro_smart_dns_config.dns_mothernode_uri, NULL, NULL)

    ZEND_INI_ENTRY_EX("quicpro.dns_mothernode_sync_interval_sec", "86400", PHP_INI_SYSTEM,
        OnUpdateDnsPositiveLong, NULL, NULL, NULL)
PHP_INI_END()

void qp_config_smart_dns_ini_register(void)
{
    REGISTER_INI_ENTRIES();
}

void qp_config_smart_dns_ini_unregister(void)
{
    UNREGISTER_INI_ENTRIES();
}
