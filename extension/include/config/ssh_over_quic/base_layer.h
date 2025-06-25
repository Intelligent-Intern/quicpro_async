/*
 * =========================================================================
 * FILENAME:   include/config/ssh_over_quic/base_layer.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    I was never more certain of how far away I was from my goal
 * than when I was standing right beside it.
 *
 * PURPOSE:
 * This header file defines the core data structure for the
 * `ssh_over_quic` configuration module.
 *
 * ARCHITECTURE:
 * This struct holds the configuration values for running the framework
 * as a secure gateway for the SSH protocol.
 * =========================================================================
 */
#ifndef QUICPRO_CONFIG_SSH_OVER_QUIC_BASE_H
#define QUICPRO_CONFIG_SSH_OVER_QUIC_BASE_H

#include "php.h"
#include <stdbool.h>

typedef struct _qp_ssh_over_quic_config_t {
    /* --- Gateway Listener Configuration --- */
    bool gateway_enable;
    char *gateway_listen_host;
    zend_long gateway_listen_port;

    /* --- Default Upstream Target --- */
    char *gateway_default_target_host;
    zend_long gateway_default_target_port;
    zend_long gateway_target_connect_timeout_ms;

    /* --- Authentication & Target Mapping --- */
    char *gateway_auth_mode;
    char *gateway_mcp_auth_agent_uri;
    char *gateway_target_mapping_mode;
    char *gateway_user_profile_agent_uri;

    /* --- Session Control & Logging --- */
    zend_long gateway_idle_timeout_sec;
    bool gateway_log_session_activity;

} qp_ssh_over_quic_config_t;

/* The single instance of this module's configuration data */
extern qp_ssh_over_quic_config_t quicpro_ssh_over_quic_config;

#endif /* QUICPRO_CONFIG_SSH_OVER_QUIC_BASE_H */
