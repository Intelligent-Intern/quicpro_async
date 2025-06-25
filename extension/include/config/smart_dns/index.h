/*
 * =========================================================================
 * FILENAME:   include/config/smart_dns/index.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    So if we are alone, it seems like an awful waste of space.
 *
 * PURPOSE:
 * This header file declares the public C-API for the Smart DNS
 * configuration module.
 *
 * ARCHITECTURE:
 * It provides the function prototypes for the module's lifecycle, which
 * are called by the master dispatcher to orchestrate loading of settings.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_SMART_DNS_INDEX_H
#define QUICPRO_CONFIG_SMART_DNS_INDEX_H

/**
 * @brief Initializes the Smart DNS configuration module.
 */
void qp_config_smart_dns_init(void);

/**
 * @brief Shuts down the Smart DNS configuration module.
 */
void qp_config_smart_dns_shutdown(void);

#endif /* QUICPRO_CONFIG_SMART_DNS_INDEX_H */
