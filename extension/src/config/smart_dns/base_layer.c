/*
 * =========================================================================
 * FILENAME:   src/config/smart_dns/base_layer.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    "Everything on the Internet starts with a name – keep it smart."
 *
 * PURPOSE:
 * Provides the single authoritative definition and memory allocation for the
 * global Smart-DNS configuration struct.
 * =========================================================================
 */

#include "include/config/smart_dns/base_layer.h"

/*
 * The single, globally accessible instance of the Smart-DNS configuration.
 */
qp_smart_dns_config_t quicpro_smart_dns_config;
