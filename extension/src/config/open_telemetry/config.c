/*
 * =========================================================================
 * FILENAME:   src/config/open_telemetry/config.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    The world is full of obvious things which nobody by any
 * chance ever observes.
 *
 * PURPOSE:
 * This file implements the logic for applying runtime configuration
 * changes to the OpenTelemetry module.
 * =========================================================================
 */

#include "include/config/open_telemetry/config.h"
#include "include/config/open_telemetry/base_layer.h"
#include "include/quicpro_globals.h"

/* Centralized validation helpers */
#include "include/validation/config_param/validate_bool.h"
#include "include/validation/config_param/validate_positive_long.h"
#include "include/validation/config_param/validate_string_from_allowlist.h"
#include "include/validation/config_param/validate_generic_string.h"
#include "include/validation/config_param/validate_double_range.h"

#include "php.h"
#include <ext/spl/spl_exceptions.h>

int qp_config_open_telemetry_apply_userland_config(zval *config_arr)
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

        if (zend_string_equals_literal(key, "enable")) {
            if (qp_validate_bool(value) != SUCCESS) return FAILURE;
            quicpro_open_telemetry_config.enable = zend_is_true(value);
        } else if (zend_string_equals_literal(key, "service_name")) {
            if (qp_validate_generic_string(value, &quicpro_open_telemetry_config.service_name) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "exporter_endpoint")) {
            if (qp_validate_generic_string(value, &quicpro_open_telemetry_config.exporter_endpoint) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "exporter_protocol")) {
            const char *allowed[] = {"grpc", "http/protobuf", NULL};
            if (qp_validate_string_from_allowlist(value, allowed, &quicpro_open_telemetry_config.exporter_protocol) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "exporter_timeout_ms")) {
            if (qp_validate_positive_long(value, &quicpro_open_telemetry_config.exporter_timeout_ms) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "exporter_headers")) {
            if (qp_validate_generic_string(value, &quicpro_open_telemetry_config.exporter_headers) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "batch_processor_max_queue_size")) {
            if (qp_validate_positive_long(value, &quicpro_open_telemetry_config.batch_processor_max_queue_size) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "batch_processor_schedule_delay_ms")) {
            if (qp_validate_positive_long(value, &quicpro_open_telemetry_config.batch_processor_schedule_delay_ms) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "traces_sampler_type")) {
            const char *allowed[] = {"parent_based_probability", "always_on", "always_off", NULL};
            if (qp_validate_string_from_allowlist(value, allowed, &quicpro_open_telemetry_config.traces_sampler_type) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "traces_sampler_ratio")) {
            if (qp_validate_double_range(value, 0.0, 1.0, &quicpro_open_telemetry_config.traces_sampler_ratio) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "traces_max_attributes_per_span")) {
            if (qp_validate_positive_long(value, &quicpro_open_telemetry_config.traces_max_attributes_per_span) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "metrics_enable")) {
            if (qp_validate_bool(value) != SUCCESS) return FAILURE;
            quicpro_open_telemetry_config.metrics_enable = zend_is_true(value);
        } else if (zend_string_equals_literal(key, "metrics_export_interval_ms")) {
            if (qp_validate_positive_long(value, &quicpro_open_telemetry_config.metrics_export_interval_ms) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "metrics_default_histogram_boundaries")) {
            if (qp_validate_generic_string(value, &quicpro_open_telemetry_config.metrics_default_histogram_boundaries) != SUCCESS) return FAILURE;
        } else if (zend_string_equals_literal(key, "logs_enable")) {
            if (qp_validate_bool(value) != SUCCESS) return FAILURE;
            quicpro_open_telemetry_config.logs_enable = zend_is_true(value);
        } else if (zend_string_equals_literal(key, "logs_exporter_batch_size")) {
            if (qp_validate_positive_long(value, &quicpro_open_telemetry_config.logs_exporter_batch_size) != SUCCESS) return FAILURE;
        }

    } ZEND_HASH_FOREACH_END();

    return SUCCESS;
}
