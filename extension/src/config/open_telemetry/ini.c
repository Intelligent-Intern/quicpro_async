/*
 * =========================================================================
 * FILENAME:   src/config/open_telemetry/ini.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    It is my business to know what other people don't know.
 *
 * PURPOSE:
 * This file implements the registration, parsing, and validation of all
 * `php.ini` settings for the OpenTelemetry module.
 * =========================================================================
 */

#include "include/config/open_telemetry/ini.h"
#include "include/config/open_telemetry/base_layer.h"
#include "include/validation/config_param/validate_comma_separated_numeric_string.h"

#include "php.h"
#include <zend_ini.h>
#include <ext/spl/spl_exceptions.h>

static ZEND_INI_MH(OnUpdateOtelPositiveLong)
{
    zend_long val = zend_atol(ZSTR_VAL(new_value), ZSTR_LEN(new_value));
    if (val <= 0) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Invalid value for an OpenTelemetry directive. A positive integer is required.");
        return FAILURE;
    }

    if (zend_string_equals_literal(entry->name, "quicpro.otel_exporter_timeout_ms")) {
        quicpro_open_telemetry_config.exporter_timeout_ms = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.otel_batch_processor_max_queue_size")) {
        quicpro_open_telemetry_config.batch_processor_max_queue_size = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.otel_batch_processor_schedule_delay_ms")) {
        quicpro_open_telemetry_config.batch_processor_schedule_delay_ms = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.otel_traces_max_attributes_per_span")) {
        quicpro_open_telemetry_config.traces_max_attributes_per_span = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.otel_metrics_export_interval_ms")) {
        quicpro_open_telemetry_config.metrics_export_interval_ms = val;
    } else if (zend_string_equals_literal(entry->name, "quicpro.otel_logs_exporter_batch_size")) {
        quicpro_open_telemetry_config.logs_exporter_batch_size = val;
    }
    return SUCCESS;
}

static ZEND_INI_MH(OnUpdateOtelSamplerRatio)
{
    double val = zend_strtod(ZSTR_VAL(new_value), NULL);
    if (val < 0.0 || val > 1.0) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Invalid value for sampler ratio. A float between 0.0 and 1.0 is required.");
        return FAILURE;
    }
    quicpro_open_telemetry_config.traces_sampler_ratio = val;
    return SUCCESS;
}

static ZEND_INI_MH(OnUpdateOtelAllowlist)
{
    const char *protocol_allowed[] = {"grpc", "http/protobuf", NULL};
    const char *sampler_allowed[] = {"parent_based_probability", "always_on", "always_off", NULL};
    const char **current_list = NULL;

    if (zend_string_equals_literal(entry->name, "quicpro.otel_exporter_protocol")) {
        current_list = protocol_allowed;
    } else if (zend_string_equals_literal(entry->name, "quicpro.otel_traces_sampler_type")) {
        current_list = sampler_allowed;
    }

    bool is_allowed = false;
    for (int i = 0; current_list[i] != NULL; i++) {
        if (strcasecmp(ZSTR_VAL(new_value), current_list[i]) == 0) {
            is_allowed = true;
            break;
        }
    }

    if (!is_allowed) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0, "Invalid value provided. The value is not one of the allowed options.");
        return FAILURE;
    }
    OnUpdateString(entry, new_value, mh_arg1, mh_arg2, mh_arg3);
    return SUCCESS;
}

static ZEND_INI_MH(OnUpdateOtelHistogramBoundaries)
{
    if (qp_validate_comma_separated_numeric_string(new_value, &quicpro_open_telemetry_config.metrics_default_histogram_boundaries) != SUCCESS) {
        return FAILURE;
    }
    return SUCCESS;
}

PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("quicpro.otel_enable", "1", PHP_INI_SYSTEM, OnUpdateBool, enable, qp_open_telemetry_config_t, quicpro_open_telemetry_config)
    STD_PHP_INI_ENTRY("quicpro.otel_service_name", "quicpro_application", PHP_INI_SYSTEM, OnUpdateString, service_name, qp_open_telemetry_config_t, quicpro_open_telemetry_config)
    STD_PHP_INI_ENTRY("quicpro.otel_exporter_endpoint", "http://localhost:4317", PHP_INI_SYSTEM, OnUpdateString, exporter_endpoint, qp_open_telemetry_config_t, quicpro_open_telemetry_config)
    ZEND_INI_ENTRY_EX("quicpro.otel_exporter_protocol", "grpc", PHP_INI_SYSTEM, OnUpdateOtelAllowlist, &quicpro_open_telemetry_config.exporter_protocol, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.otel_exporter_timeout_ms", "10000", PHP_INI_SYSTEM, OnUpdateOtelPositiveLong, NULL, NULL, NULL)
    STD_PHP_INI_ENTRY("quicpro.otel_exporter_headers", "", PHP_INI_SYSTEM, OnUpdateString, exporter_headers, qp_open_telemetry_config_t, quicpro_open_telemetry_config)
    ZEND_INI_ENTRY_EX("quicpro.otel_batch_processor_max_queue_size", "2048", PHP_INI_SYSTEM, OnUpdateOtelPositiveLong, NULL, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.otel_batch_processor_schedule_delay_ms", "5000", PHP_INI_SYSTEM, OnUpdateOtelPositiveLong, NULL, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.otel_traces_sampler_type", "parent_based_probability", PHP_INI_SYSTEM, OnUpdateOtelAllowlist, &quicpro_open_telemetry_config.traces_sampler_type, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.otel_traces_sampler_ratio", "1.0", PHP_INI_SYSTEM, OnUpdateOtelSamplerRatio, NULL, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.otel_traces_max_attributes_per_span", "128", PHP_INI_SYSTEM, OnUpdateOtelPositiveLong, NULL, NULL, NULL)
    STD_PHP_INI_ENTRY("quicpro.otel_metrics_enable", "1", PHP_INI_SYSTEM, OnUpdateBool, metrics_enable, qp_open_telemetry_config_t, quicpro_open_telemetry_config)
    ZEND_INI_ENTRY_EX("quicpro.otel_metrics_export_interval_ms", "60000", PHP_INI_SYSTEM, OnUpdateOtelPositiveLong, NULL, NULL, NULL)
    ZEND_INI_ENTRY_EX("quicpro.otel_metrics_default_histogram_boundaries", "0,5,10,25,50,75,100,250,500,1000", PHP_INI_SYSTEM, OnUpdateOtelHistogramBoundaries, NULL, NULL, NULL)
    STD_PHP_INI_ENTRY("quicpro.otel_logs_enable", "0", PHP_INI_SYSTEM, OnUpdateBool, logs_enable, qp_open_telemetry_config_t, quicpro_open_telemetry_config)
    ZEND_INI_ENTRY_EX("quicpro.otel_logs_exporter_batch_size", "512", PHP_INI_SYSTEM, OnUpdateOtelPositiveLong, NULL, NULL, NULL)
PHP_INI_END()

void qp_config_open_telemetry_ini_register(void) { REGISTER_INI_ENTRIES(); }
void qp_config_open_telemetry_ini_unregister(void) { UNREGISTER_INI_ENTRIES(); }
