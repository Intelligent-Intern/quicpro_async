/*
 * =========================================================================
 * FILENAME:   src/config/smart_dns/index.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    "DNS – It's not just a name service; it's the phone book of cyberspace."
 *
 * PURPOSE:
 * Implements the lifecycle functions (init/shutdown) for the Smart-DNS
 * configuration module. During startup we load hard-coded, conservative
 * defaults first and then allow php.ini to selectively override them.
 * =========================================================================
 */

#include "include/config/smart_dns/index.h"
#include "include/config/smart_dns/default.h"
#include "include/config/smart_dns/ini.h"

#include "php.h"

/**
 * @brief Initializes the Smart-DNS configuration module.
 */
void qp_config_smart_dns_init(void)
{
    /* Step 1: Load safe, hard-coded defaults */
    qp_config_smart_dns_defaults_load();

    /* Step 2: Register php.ini directives to override the defaults */
    qp_config_smart_dns_ini_register();
}

/**
 * @brief Shuts down the Smart-DNS configuration module.
 */
void qp_config_smart_dns_shutdown(void)
{
    /* Unregister php.ini directives to clean up global state */
    qp_config_smart_dns_ini_unregister();
}
