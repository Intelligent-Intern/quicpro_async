/*
 * =========================================================================
 * FILENAME:   include/config/ssh_over_quic/config.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    You want to know how I did it? This is how I did it, Anton:
 * I never saved anything for the swim back.
 *
 * PURPOSE:
 * This header file declares the public C-API for applying runtime
 * configuration changes from PHP userland to the SSH over QUIC module.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_SSH_OVER_QUIC_CONFIG_H
#define QUICPRO_CONFIG_SSH_OVER_QUIC_CONFIG_H

#include "php.h"

/**
 * @brief Applies runtime configuration settings from a PHP array.
 * @param config_arr A zval pointer to a PHP array containing the key-value
 * pairs of the configuration to apply.
 * @return `SUCCESS` if all settings were successfully validated and applied,
 * `FAILURE` otherwise.
 */
int qp_config_ssh_over_quic_apply_userland_config(zval *config_arr);

#endif /* QUICPRO_CONFIG_SSH_OVER_QUIC_CONFIG_H */
