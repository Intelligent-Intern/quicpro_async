/*
 * =========================================================================
 * FILENAME:   src/config/open_telemetry/default.c
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    There is nothing more deceptive than an obvious fact.
 *
 * PURPOSE:
 * This file implements the function that loads the hardcoded, default
 * values into the OpenTelemetry configuration struct.
 * =========================================================================
 */

#include "include/config/open_telemetry/default.h"
#include "include/config/open_telemetry/base_layer.h"
#include "php.h"

/**
 * @brief Loads the hardcoded, default values into the module's config struct.
 */
void qp_config_open_telemetry_defaults_load(void)
{
    /* --- General & Exporter Settings --- */
    quicpro_open_telemetry_config.enable = true;
    quicpro_open_telemetry_config.service_name = pestrdup("quicpro_application", 1);
    quicpro_open_telemetry_config.exporter_endpoint = pestrdup("http://localhost:4317", 1);
    quicpro_open_telemetry_config.exporter_protocol = pestrdup("grpc", 1);
    quicpro_open_telemetry_config.exporter_timeout_ms = 10000;
    quicpro_open_telemetry_config.exporter_headers = pestrdup("", 1);

    /* --- Batch Processor Settings --- */
    quicpro_open_telemetry_config.batch_processor_max_queue_size = 2048;
    quicpro_open_telemetry_config.batch_processor_schedule_delay_ms = 5000;

    /* --- Tracing Settings --- */
    quicpro_open_telemetry_config.traces_sampler_type = pestrdup("parent_based_probability", 1);
    quicpro_open_telemetry_config.traces_sampler_ratio = 1.0;
    quicpro_open_telemetry_config.traces_max_attributes_per_span = 128;

    /* --- Metrics Settings --- */
    quicpro_open_telemetry_config.metrics_enable = true;
    quicpro_open_telemetry_config.metrics_export_interval_ms = 60000;
    quicpro_open_telemetry_config.metrics_default_histogram_boundaries = pestrdup("0,5,10,25,50,75,100,250,500,1000", 1);

    /* --- Logging Settings (Experimental) --- */
    quicpro_open_telemetry_config.logs_enable = false;
    quicpro_open_telemetry_config.logs_exporter_batch_size = 512;
}
