/*
 * =========================================================================
 * FILENAME:   src/config/dynamic_admin_api/ini.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    You're not a wartime consigliere, Tom. Things could get
 * rough with the Tattaglia Family.
 *
 * PURPOSE:
 * This file implements the registration, parsing, and validation of all
 * `php.ini` settings for the dynamic admin API module.
 * =========================================================================
 */

#include "include/config/dynamic_admin_api/ini.h"
#include "include/config/dynamic_admin_api/base_layer.h"
#include "main/php_streams.h" /* For VCWD_ACCESS */
#include "php.h"
#include <zend_ini.h>
#include <ext/spl/spl_exceptions.h>

/* Custom OnUpdate handler for the API port to validate its range. */
static ZEND_INI_MH(OnUpdateAdminApiPort)
{
    zend_long val = zend_atol(ZSTR_VAL(new_value), ZSTR_LEN(new_value));
    if (val < 1024 || val > 65535) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Invalid port for Admin API. Must be between 1024 and 65535.");
        return FAILURE;
    }
    quicpro_dynamic_admin_api_config.port = val;
    return SUCCESS;
}

/* Custom OnUpdate handler to validate the authentication mode. */
static ZEND_INI_MH(OnUpdateAdminAuthMode)
{
    if (strcasecmp(ZSTR_VAL(new_value), "mtls") != 0) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Invalid auth mode for Admin API. Only 'mtls' is supported.");
        return FAILURE;
    }
    OnUpdateString(entry, new_value, mh_arg1, mh_arg2, mh_arg3);
    return SUCCESS;
}

/* Custom OnUpdate handler to validate file paths are readable. */
static ZEND_INI_MH(OnUpdateAdminApiFilePath)
{
    char *path = ZSTR_VAL(new_value);
    if (ZSTR_LEN(new_value) > 0 && VCWD_ACCESS(path, R_OK) != 0) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Provided file path for Admin API is not accessible or does not exist.");
        return FAILURE;
    }
    OnUpdateString(entry, new_value, mh_arg1, mh_arg2, mh_arg3);
    return SUCCESS;
}


PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("quicpro.admin_api_bind_host", "127.0.0.1", PHP_INI_SYSTEM, OnUpdateString, bind_host, qp_dynamic_admin_api_config_t, quicpro_dynamic_admin_api_config)
    ZEND_INI_ENTRY_EX("quicpro.admin_api_port", "2019", PHP_INI_SYSTEM, OnUpdateAdminApiPort, NULL, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.admin_api_auth_mode", "mtls", PHP_INI_SYSTEM, OnUpdateAdminAuthMode, &quicpro_dynamic_admin_api_config.auth_mode, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.admin_api_ca_file", "", PHP_INI_SYSTEM, OnUpdateAdminApiFilePath, &quicpro_dynamic_admin_api_config.ca_file, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.admin_api_cert_file", "", PHP_INI_SYSTEM, OnUpdateAdminApiFilePath, &quicpro_dynamic_admin_api_config.cert_file, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.admin_api_key_file", "", PHP_INI_SYSTEM, OnUpdateAdminApiFilePath, &quicpro_dynamic_admin_api_config.key_file, NULL, NULL)
PHP_INI_END()

/**
 * @brief Registers this module's php.ini entries with the Zend Engine.
 */
void qp_config_dynamic_admin_api_ini_register(void)
{
    REGISTER_INI_ENTRIES();
}

/**
 * @brief Unregisters this module's php.ini entries from the Zend Engine.
 */
void qp_config_dynamic_admin_api_ini_unregister(void)
{
    UNREGISTER_INI_ENTRIES();
}
