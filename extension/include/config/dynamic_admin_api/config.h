/*
 * =========================================================================
 * FILENAME:   include/config/dynamic_admin_api/config.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    I have a sentimental weakness for my children and I spoil them,
 * as you can see. They talk when they should listen.
 *
 * PURPOSE:
 * This header file declares the public C-API for applying runtime
 * configuration changes from PHP userland to the dynamic admin API module.
 * =========================================================================
 */

#ifndef QUICPRO_CONFIG_DYNAMIC_ADMIN_API_CONFIG_H
#define QUICPRO_CONFIG_DYNAMIC_ADMIN_API_CONFIG_H

#include "php.h"

/**
 * @brief Applies runtime configuration settings from a PHP array.
 * @param config_arr A zval pointer to a PHP array containing the key-value
 * pairs of the configuration to apply.
 * @return `SUCCESS` if all settings were successfully validated and applied,
 * `FAILURE` otherwise.
 */
int qp_config_dynamic_admin_api_apply_userland_config(zval *config_arr);

#endif /* QUICPRO_CONFIG_DYNAMIC_ADMIN_API_CONFIG_H */
