/*
 * =========================================================================
 * FILENAME:   src/config/cloud_autoscale/ini.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    There is no escape—we pay for the violence of our ancestors.
 *
 * PURPOSE:
 * This file implements the registration, parsing, and validation of all
 * `php.ini` settings for the cloud autoscaling module.
 * =========================================================================
 */

#include "include/config/cloud_autoscale/ini.h"
#include "include/config/cloud_autoscale/base_layer.h"
#include "php.h"
#include <zend_ini.h>
#include <ext/spl/spl_exceptions.h>

/* Custom OnUpdate handler for positive integer values */
static ZEND_INI_MH(OnUpdateAutoscalePositiveLong)
{
    zend_long val = zend_atol(ZSTR_VAL(new_value), ZSTR_LEN(new_value));

    if (val <= 0) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Invalid value for an autoscale directive. A positive integer is required.");
        return FAILURE;
    }

    if (zend_string_equals_literal(entry->name, "quicpro.cluster_autoscale_min_nodes")) {
        quicpro_cloud_autoscale_config.min_nodes = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.cluster_autoscale_max_nodes")) {
        quicpro_cloud_autoscale_config.max_nodes = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.cluster_autoscale_cooldown_period_sec")) {
        quicpro_cloud_autoscale_config.cooldown_period_sec = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.cluster_autoscale_idle_node_timeout_sec")) {
        quicpro_cloud_autoscale_config.idle_node_timeout_sec = val;
    }

    return SUCCESS;
}

/* Custom OnUpdate handler for percentage values (0-100) */
static ZEND_INI_MH(OnUpdateAutoscalePercent)
{
    zend_long val = zend_atol(ZSTR_VAL(new_value), ZSTR_LEN(new_value));

    if (val < 0 || val > 100) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Invalid value for an autoscale threshold directive. An integer between 0 and 100 is required.");
        return FAILURE;
    }

    if (zend_string_equals_literal(entry->name, "quicpro.cluster_autoscale_scale_up_cpu_threshold_percent")) {
        quicpro_cloud_autoscale_config.scale_up_cpu_threshold_percent = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.cluster_autoscale_scale_down_cpu_threshold_percent")) {
        quicpro_cloud_autoscale_config.scale_down_cpu_threshold_percent = val;
    }

    return SUCCESS;
}

/* Custom OnUpdate handler for the provider string */
static ZEND_INI_MH(OnUpdateAutoscaleProvider)
{
    const char *allowed[] = {"aws", "azure", "hetzner", "google_cloud", "digitalocean", "", NULL};
    bool is_allowed = false;
    for (int i = 0; allowed[i] != NULL; i++) {
        if (strcasecmp(ZSTR_VAL(new_value), allowed[i]) == 0) {
            is_allowed = true;
            break;
        }
    }

    if (!is_allowed) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Invalid value for autoscale provider. Unsupported provider specified.");
        return FAILURE;
    }
    OnUpdateString(entry, new_value, mh_arg1, mh_arg2, mh_arg3);
    return SUCCESS;
}

PHP_INI_BEGIN()
    /* --- Provider & Credentials --- */
    ZEND_INI_ENTRY_EX("quicpro.cluster_autoscale_provider", "", PHP_INI_SYSTEM, OnUpdateAutoscaleProvider, &quicpro_cloud_autoscale_config.provider, NULL, NULL)
    STD_PHP_INI_ENTRY("quicpro.cluster_autoscale_region", "", PHP_INI_SYSTEM, OnUpdateString, region, qp_cloud_autoscale_config_t, quicpro_cloud_autoscale_config)
    STD_PHP_INI_ENTRY("quicpro.cluster_autoscale_credentials_path", "", PHP_INI_SYSTEM, OnUpdateString, credentials_path, qp_cloud_autoscale_config_t, quicpro_cloud_autoscale_config)

    /* --- Scaling Triggers & Policy --- */
    ZEND_INI_ENTRY_EX("quicpro.cluster_autoscale_min_nodes", "1", PHP_INI_SYSTEM, OnUpdateAutoscalePositiveLong, NULL, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.cluster_autoscale_max_nodes", "1", PHP_INI_SYSTEM, OnUpdateAutoscalePositiveLong, NULL, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.cluster_autoscale_scale_up_cpu_threshold_percent", "80", PHP_INI_SYSTEM, OnUpdateAutoscalePercent, NULL, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.cluster_autoscale_scale_down_cpu_threshold_percent", "20", PHP_INI_SYSTEM, OnUpdateAutoscalePercent, NULL, NULL, NULL)
    STD_PHP_INI_ENTRY("quicpro.cluster_autoscale_scale_up_policy", "add_nodes:1", PHP_INI_SYSTEM, OnUpdateString, scale_up_policy, qp_cloud_autoscale_config_t, quicpro_cloud_autoscale_config)
    ZEND_INI_ENTRY_EX("quicpro.cluster_autoscale_cooldown_period_sec", "300", PHP_INI_SYSTEM, OnUpdateAutoscalePositiveLong, NULL, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.cluster_autoscale_idle_node_timeout_sec", "600", PHP_INI_SYSTEM, OnUpdateAutoscalePositiveLong, NULL, NULL, NULL)

    /* --- Node Provisioning --- */
    STD_PHP_INI_ENTRY("quicpro.cluster_autoscale_instance_type", "", PHP_INI_SYSTEM, OnUpdateString, instance_type, qp_cloud_autoscale_config_t, quicpro_cloud_autoscale_config)
    STD_PHP_INI_ENTRY("quicpro.cluster_autoscale_instance_image_id", "", PHP_INI_SYSTEM, OnUpdateString, instance_image_id, qp_cloud_autoscale_config_t, quicpro_cloud_autoscale_config)
    STD_PHP_INI_ENTRY("quicpro.cluster_autoscale_network_config", "", PHP_INI_SYSTEM, OnUpdateString, network_config, qp_cloud_autoscale_config_t, quicpro_cloud_autoscale_config)
    STD_PHP_INI_ENTRY("quicpro.cluster_autoscale_instance_tags", "", PHP_INI_SYSTEM, OnUpdateString, instance_tags, qp_cloud_autoscale_config_t, quicpro_cloud_autoscale_config)
PHP_INI_END()

void qp_config_cloud_autoscale_ini_register(void) { REGISTER_INI_ENTRIES(); }
void qp_config_cloud_autoscale_ini_unregister(void) { UNREGISTER_INI_ENTRIES(); }
