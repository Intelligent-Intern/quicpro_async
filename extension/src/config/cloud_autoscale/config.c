/*
 * =========================================================================
 * FILENAME:   src/config/cloud_autoscale/config.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Without change, something sleeps inside us, and seldom
 * awakens. The sleeper must awaken.
 *
 * PURPOSE:
 * This file implements the logic for applying runtime configuration
 * changes to the cloud autoscaling module.
 * =========================================================================
 */

#include "include/config/cloud_autoscale/config.h"
#include "include/config/cloud_autoscale/base_layer.h"
#include "include/quicpro_globals.h"

/* Centralized validation helpers */
#include "include/validation/config_param/validate_string_from_allowlist.h"
#include "include/validation/config_param/validate_positive_long.h"
#include "include/validation/config_param/validate_long_range.h"
#include "include/validation/config_param/validate_generic_string.h"
#include "include/validation/config_param/validate_scale_up_policy_string.h"

#include "php.h"
#include <ext/spl/spl_exceptions.h>

int qp_config_cloud_autoscale_apply_userland_config(zval *config_arr)
{
    zval *value;
    zend_string *key;

    if (!quicpro_globals.is_userland_override_allowed) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Configuration override from userland is disabled by system administrator.");
        return FAILURE;
    }

    if (Z_TYPE_P(config_arr) != IS_ARRAY) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Configuration must be provided as an array.");
        return FAILURE;
    }

    ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(config_arr), key, value) {
        if (!key) continue;

        if (zend_string_equals_literal(key, "provider")) {
            const char *allowed[] = {"aws", "azure", "hetzner", "google_cloud", "digitalocean", "", NULL};
            if (qp_validate_string_from_allowlist(value, allowed, &quicpro_cloud_autoscale_config.provider) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "region")) {
            if (qp_validate_generic_string(value, &quicpro_cloud_autoscale_config.region) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "credentials_path")) {
            if (qp_validate_generic_string(value, &quicpro_cloud_autoscale_config.credentials_path) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "min_nodes")) {
            if (qp_validate_positive_long(value, &quicpro_cloud_autoscale_config.min_nodes) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "max_nodes")) {
            if (qp_validate_positive_long(value, &quicpro_cloud_autoscale_config.max_nodes) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "scale_up_cpu_threshold_percent")) {
            if (qp_validate_long_range(value, 0, 100, &quicpro_cloud_autoscale_config.scale_up_cpu_threshold_percent) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "scale_down_cpu_threshold_percent")) {
            if (qp_validate_long_range(value, 0, 100, &quicpro_cloud_autoscale_config.scale_down_cpu_threshold_percent) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "scale_up_policy")) {
            if (qp_validate_scale_up_policy_string(value, &quicpro_cloud_autoscale_config.scale_up_policy) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "cooldown_period_sec")) {
            if (qp_validate_positive_long(value, &quicpro_cloud_autoscale_config.cooldown_period_sec) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "idle_node_timeout_sec")) {
            if (qp_validate_positive_long(value, &quicpro_cloud_autoscale_config.idle_node_timeout_sec) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "instance_type")) {
            if (qp_validate_generic_string(value, &quicpro_cloud_autoscale_config.instance_type) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "instance_image_id")) {
            if (qp_validate_generic_string(value, &quicpro_cloud_autoscale_config.instance_image_id) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "network_config")) {
            if (qp_validate_generic_string(value, &quicpro_cloud_autoscale_config.network_config) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "instance_tags")) {
            if (qp_validate_generic_string(value, &quicpro_cloud_autoscale_config.instance_tags) != SUCCESS) return FAILURE;
        }

    } ZEND_HASH_FOREACH_END();

    return SUCCESS;
}
