/*
 * =========================================================================
 * FILENAME:   include/config/dynamic_admin_api/base_layer.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Some day, and that day may never come, I will call upon you
 * to do a service for me.
 *
 * PURPOSE:
 * This header file defines the core data structure for the
 * `dynamic_admin_api` configuration module.
 *
 * ARCHITECTURE:
 * This struct holds the configuration values required to bootstrap the
 * high-security Admin API endpoint.
 * =========================================================================
 */
#ifndef QUICPRO_CONFIG_DYNAMIC_ADMIN_API_BASE_H
#define QUICPRO_CONFIG_DYNAMIC_ADMIN_API_BASE_H

#include "php.h"
#include <stdbool.h>

typedef struct _qp_dynamic_admin_api_config_t {
    /*
     * NOTE: The 'admin_api_enable' flag is intentionally located in the
     * `security_and_traffic` module to centralize security policy control.
     * This module will read that configuration to decide whether to activate.
     */
    char *bind_host;
    zend_long port;
    char *auth_mode;
    char *ca_file;
    char *cert_file;
    char *key_file;

} qp_dynamic_admin_api_config_t;

/* The single instance of this module's configuration data */
extern qp_dynamic_admin_api_config_t quicpro_dynamic_admin_api_config;

#endif /* QUICPRO_CONFIG_DYNAMIC_ADMIN_API_BASE_H */
