/*
 * =========================================================================
 * FILENAME:   include/config/security_and_traffic/base_layer.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    We're no strangers to love...
 *
 * PURPOSE:
 * This header file defines the core data structure for the
 * `security_and_traffic` configuration module.
 *
 * ARCHITECTURE:
 * This struct holds the actual configuration values for this module after
 * they have been loaded from defaults, `php.ini`, and potentially
 * runtime configuration. The `extern` declaration makes the single global
 * instance of this struct available to all files within the module.
 * =========================================================================
 */
#ifndef QUICPRO_CONFIG_SECURITY_BASE_H
#define QUICPRO_CONFIG_SECURITY_BASE_H

#include "php.h"
#include <stdbool.h>

typedef struct _qp_security_config_t {
    /* --- Core Override Policy --- */
    bool allow_config_override;
    bool admin_api_enable;

    /* --- Rate Limiter --- */
    bool rate_limiter_enable;
    zend_long rate_limiter_requests_per_sec;
    zend_long rate_limiter_burst;

    /* --- CORS --- */
    char *cors_allowed_origins;

} qp_security_config_t;

/* The single instance of this module's configuration data */
extern qp_security_config_t quicpro_security_config;

#endif /* QUICPRO_CONFIG_SECURITY_BASE_H */
