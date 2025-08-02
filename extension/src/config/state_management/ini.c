/*
 * =========================================================================
 * FILENAME:   src/config/state_management/ini.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    "Without data, you are just another person with an opinion." — W. Edwards Deming
 *
 * PURPOSE:
 *   Registers, parses and validates all `php.ini` directives for the
 *   State‑Management configuration module.
 * =========================================================================
 */

#include "include/config/state_management/ini.h"
#include "include/config/state_management/base_layer.h"

#include "php.h"
#include <zend_ini.h>
#include <ext/spl/spl_exceptions.h>

/* Allow‑list validation for backends */
static ZEND_INI_MH(OnUpdateStateMgmtBackend)
{
    const char *allowed[] = {"memory", "sqlite", "redis", "postgres", NULL};
    int i;
    for (i = 0; allowed[i]; ++i) {
        if (zend_string_equals_literal(new_value, allowed[i])) {
            quicpro_state_mgmt_config.default_backend = estrdup(ZSTR_VAL(new_value));
            return SUCCESS;
        }
    }
    zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0,
        "Invalid state_manager_default_backend. Allowed values: memory, sqlite, redis, postgres");
    return FAILURE;
}

/* Generic string copy */
static ZEND_INI_MH(OnUpdateStateMgmtString)
{
    OnUpdateString(entry, new_value, mh_arg1, mh_arg2, mh_arg3);
    return SUCCESS;
}

PHP_INI_BEGIN()
    ZEND_INI_ENTRY_EX("quicpro.state_manager_default_backend", "memory",
        PHP_INI_SYSTEM, OnUpdateStateMgmtBackend, NULL, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.state_manager_default_uri", "",
        PHP_INI_SYSTEM, OnUpdateStateMgmtString,
        &quicpro_state_mgmt_config.default_uri, NULL, NULL)
PHP_INI_END()

void qp_config_state_management_ini_register(void)
{
    REGISTER_INI_ENTRIES();
}

void qp_config_state_management_ini_unregister(void)
{
    UNREGISTER_INI_ENTRIES();
}
