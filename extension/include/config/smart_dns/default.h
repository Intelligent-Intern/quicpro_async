/*
 * =========================================================================
 * FILENAME:   include/config/smart_dns/default.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    I'll tell you one thing about the universe, though. The universe
 * is a very strange place.
 *
 * PURPOSE:
 * This header declares the function for loading the module's hardcoded
 * default values for the Smart DNS server.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_SMART_DNS_DEFAULT_H
#define QUICPRO_CONFIG_SMART_DNS_DEFAULT_H

/**
 * @brief Loads the hardcoded, default values into the module's config struct.
 */
void qp_config_smart_dns_defaults_load(void);

#endif /* QUICPRO_CONFIG_SMART_DNS_DEFAULT_H */
