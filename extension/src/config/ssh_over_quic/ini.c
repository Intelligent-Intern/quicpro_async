/*
 * =========================================================================
 * FILENAME:   src/config/ssh_over_quic/ini.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    "Remember, all I'm offering is the truth. Nothing more."
 *
 * PURPOSE:
 *   Registers, parses and validates all `php.ini` directives for the
 *   SSH-over-QUIC gateway configuration module.
 * =========================================================================
 */

#include "include/config/ssh_over_quic/ini.h"
#include "include/config/ssh_over_quic/base_layer.h"

#include "php.h"
#include <zend_ini.h>
#include <ext/spl/spl_exceptions.h>

/* -------------------------------------------------------------------------
 * Utility validation helpers
 * -------------------------------------------------------------------------*/

/* Positive long validation */
static ZEND_INI_MH(OnUpdateSshQuicPositiveLong)
{
    zend_long val = zend_atol(ZSTR_VAL(new_value), ZSTR_LEN(new_value));

    if (val <= 0) {
        zend_throw_exception_ex(
            spl_ce_InvalidArgumentException, 0,
            "Invalid value for SSH-over-QUIC directive. A positive integer is required.");
        return FAILURE;
    }

    if      (zend_string_equals_literal(entry->name, "quicpro.ssh_gateway_listen_port"))              quicpro_ssh_quic_config.listen_port               = val;
    else if (zend_string_equals_literal(entry->name, "quicpro.ssh_gateway_default_target_port"))      quicpro_ssh_quic_config.default_target_port       = val;
    else if (zend_string_equals_literal(entry->name, "quicpro.ssh_gateway_target_connect_timeout_ms"))quicpro_ssh_quic_config.target_connect_timeout_ms = val;
    else if (zend_string_equals_literal(entry->name, "quicpro.ssh_gateway_idle_timeout_sec"))         quicpro_ssh_quic_config.idle_timeout_sec          = val;
    return SUCCESS;
}

/* String allow-list validation for auth_mode */
static ZEND_INI_MH(OnUpdateSshQuicAuthMode)
{
    const char *allowed[] = {"mtls", "mcp_token", NULL};
    for (int i = 0; allowed[i]; ++i) {
        if (zend_string_equals_literal(new_value, allowed[i])) {
            quicpro_ssh_quic_config.gateway_auth_mode = estrdup(ZSTR_VAL(new_value));
            return SUCCESS;
        }
    }
    zend_throw_exception_ex(
        spl_ce_InvalidArgumentException, 0,
        "Invalid ssh_gateway_auth_mode. Allowed values: mtls, mcp_token");
    return FAILURE;
}

/* String allow-list validation for target_mapping_mode */
static ZEND_INI_MH(OnUpdateSshQuicMappingMode)
{
    const char *allowed[] = {"static", "user_profile", NULL};
    for (int i = 0; allowed[i]; ++i) {
        if (zend_string_equals_literal(new_value, allowed[i])) {
            quicpro_ssh_quic_config.target_mapping_mode = estrdup(ZSTR_VAL(new_value));
            return SUCCESS;
        }
    }
    zend_throw_exception_ex(
        spl_ce_InvalidArgumentException, 0,
        "Invalid ssh_gateway_target_mapping_mode. Allowed: static, user_profile");
    return FAILURE;
}

/* Generic duplicate for free string options */
static ZEND_INI_MH(OnUpdateSshQuicStringDuplicate)
{
    OnUpdateString(entry, new_value, mh_arg1, mh_arg2, mh_arg3);
    return SUCCESS;
}

/* -------------------------------------------------------------------------
 * Directive table
 * -------------------------------------------------------------------------*/
PHP_INI_BEGIN()
    /* Master switch */
    STD_PHP_INI_ENTRY("quicpro.ssh_gateway_enable", "0", PHP_INI_SYSTEM,
        OnUpdateBool, gateway_enable, qp_ssh_quic_config_t, quicpro_ssh_quic_config)

    /* Listener address */
    ZEND_INI_ENTRY_EX("quicpro.ssh_gateway_listen_host", "0.0.0.0",
        PHP_INI_SYSTEM, OnUpdateSshQuicStringDuplicate, &quicpro_ssh_quic_config.listen_host, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.ssh_gateway_listen_port", "2222",
        PHP_INI_SYSTEM, OnUpdateSshQuicPositiveLong, NULL, NULL, NULL)

    /* Default upstream target */
    ZEND_INI_ENTRY_EX("quicpro.ssh_gateway_default_target_host", "127.0.0.1",
        PHP_INI_SYSTEM, OnUpdateSshQuicStringDuplicate, &quicpro_ssh_quic_config.default_target_host, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.ssh_gateway_default_target_port", "22",
        PHP_INI_SYSTEM, OnUpdateSshQuicPositiveLong, NULL, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.ssh_gateway_target_connect_timeout_ms", "5000",
        PHP_INI_SYSTEM, OnUpdateSshQuicPositiveLong, NULL, NULL, NULL)

    /* Authentication & target mapping */
    ZEND_INI_ENTRY_EX("quicpro.ssh_gateway_auth_mode", "mtls",
        PHP_INI_SYSTEM, OnUpdateSshQuicAuthMode, NULL, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.ssh_gateway_mcp_auth_agent_uri", "",
        PHP_INI_SYSTEM, OnUpdateSshQuicStringDuplicate, &quicpro_ssh_quic_config.mcp_auth_agent_uri, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.ssh_gateway_target_mapping_mode", "static",
        PHP_INI_SYSTEM, OnUpdateSshQuicMappingMode, NULL, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.ssh_gateway_user_profile_agent_uri", "",
        PHP_INI_SYSTEM, OnUpdateSshQuicStringDuplicate, &quicpro_ssh_quic_config.user_profile_agent_uri, NULL, NULL)

    /* Session control */
    ZEND_INI_ENTRY_EX("quicpro.ssh_gateway_idle_timeout_sec", "1800",
        PHP_INI_SYSTEM, OnUpdateSshQuicPositiveLong, NULL, NULL, NULL)
    STD_PHP_INI_ENTRY("quicpro.ssh_gateway_log_session_activity", "1", PHP_INI_SYSTEM,
        OnUpdateBool, log_session_activity, qp_ssh_quic_config_t, quicpro_ssh_quic_config)
PHP_INI_END()

/* -------------------------------------------------------------------------
 * Register / Unregister helpers
 * -------------------------------------------------------------------------*/
void qp_config_ssh_over_quic_ini_register(void)
{
    REGISTER_INI_ENTRIES();
}

void qp_config_ssh_over_quic_ini_unregister(void)
{
    UNREGISTER_INI_ENTRIES();
}