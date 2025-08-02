/* extension/include/server/open_telemetry.h */

#ifndef QUICPRO_SERVER_OPEN_TELEMETRY_H
#define QUICPRO_SERVER_OPEN_TELEMETRY_H

#include <php.h>

/**
 * @file extension/include/server/open_telemetry.h
 * @brief Public API declarations for server-side OpenTelemetry integration.
 *
 * This header defines the interface for Quicpro's native OpenTelemetry (OTel)
 * support. It enables the automatic generation and propagation of distributed
 * traces, metrics, and logs for all server operations. This provides deep,
 * out-of-the-box observability into the performance and behavior of
 * applications built with the framework, with minimal developer effort.
 */

/**
 * @brief Initializes the OpenTelemetry exporter for a server instance.
 *
 * This function configures and starts the OTel exporter based on the provided
 * configuration. Once initialized, the server's C-core will automatically
 * instrument incoming requests, creating spans for the request lifecycle and
 * propagating trace contexts. It will also collect and periodically export
 * internal performance metrics.
 *
 * This should typically be called once at server startup.
 *
 * @param server_resource The resource of the server instance to be instrumented.
 * @param config_resource A PHP resource representing a `Quicpro\Config` object.
 * The configuration must contain the `otel_*` directives, such as
 * `otel_service_name` and `otel_exporter_endpoint`.
 * @return TRUE on successful initialization of the OTel exporter.
 * FALSE on failure, throwing a `Quicpro\Exception\ConfigException` if the
 * OTel configuration is incomplete or invalid.
 */
PHP_FUNCTION(quicpro_server_init_telemetry);

#endif // QUICPRO_SERVER_OPEN_TELEMETRY_H
