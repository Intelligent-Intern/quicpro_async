/*
 * =========================================================================
 * FILENAME:   include/config/open_telemetry/base_layer.h
 * PROJECT:    quicpro_async
 * AUTHOR:     Jochen Schultz <jschultz@php.net>
 *
 * WELCOME:    Data! Data! Data! I can't make bricks without clay.
 *
 * PURPOSE:
 * This header file defines the core data structure for the
 * `open_telemetry` configuration module.
 *
 * ARCHITECTURE:
 * This struct holds the configuration values for the native, C-level
 * OpenTelemetry (OTel) integration for generating and propagating
 * distributed traces, metrics, and logs.
 * =========================================================================
 */
#ifndef QUICPRO_CONFIG_OPEN_TELEMETRY_BASE_H
#define QUICPRO_CONFIG_OPEN_TELEMETRY_BASE_H

#include "php.h"
#include <stdbool.h>

typedef struct _qp_open_telemetry_config_t {
    /* --- General & Exporter Settings --- */
    bool enable;
    char *service_name;
    char *exporter_endpoint;
    char *exporter_protocol;
    zend_long exporter_timeout_ms;
    char *exporter_headers;

    /* --- Batch Processor Settings --- */
    zend_long batch_processor_max_queue_size;
    zend_long batch_processor_schedule_delay_ms;

    /* --- Tracing Settings --- */
    char *traces_sampler_type;
    double traces_sampler_ratio;
    zend_long traces_max_attributes_per_span;

    /* --- Metrics Settings --- */
    bool metrics_enable;
    zend_long metrics_export_interval_ms;
    char *metrics_default_histogram_boundaries;

    /* --- Logging Settings (Experimental) --- */
    bool logs_enable;
    zend_long logs_exporter_batch_size;

} qp_open_telemetry_config_t;

/* The single instance of this module's configuration data */
extern qp_open_telemetry_config_t quicpro_open_telemetry_config;

#endif /* QUICPRO_CONFIG_OPEN_TELEMETRY_BASE_H */
