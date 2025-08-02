/*
 * extension/include/php_quicpro_arginfo.h
 *
 * Argument information for all PHP methods provided by the quicpro_async
 * extension. This file defines the public API signature for all classes.
 *
 * Each ZEND_BEGIN_ARG_INFO_EX block corresponds to a method and is used by
 * the Zend Engine for type checking, reflection, and documentation.
 */

#ifndef PHP_QUICPRO_ARGINFO_H
#define PHP_QUICPRO_ARGINFO_H

/* ============================================================================== */
/* == Quicpro\Config Class                                                     == */
/* ============================================================================== */

/* {{{ Quicpro\Config::new(?array $options): resource */
ZEND_BEGIN_ARG_INFO_EX(arginfo_Quicpro_Config_new, 0, 0, 0)
    ZEND_ARG_TYPE_INFO(0, options, IS_ARRAY, 1)
ZEND_END_ARG_INFO()
/* }}} */


/* ============================================================================== */
/* == Quicpro\MCP Class (Client)                                               == */
/* ============================================================================== */

/* {{{ Quicpro\MCP::__construct(string $host, int $port, resource $config, ?array $options = null) */
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_Quicpro_MCP___construct, 0, 3, IS_VOID, 0)
    ZEND_ARG_TYPE_INFO(0, host,    IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, port,    IS_LONG,   0)
    ZEND_ARG_INFO(0, config) /* resource */
    ZEND_ARG_TYPE_INFO(0, options, IS_ARRAY,  1)
ZEND_END_ARG_INFO()
/* }}} */

/* {{{ Quicpro\MCP::close(): void */
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_Quicpro_MCP_close, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()
/* }}} */

/* {{{ Quicpro\MCP::request(string $service_name, string $method_name, string $request_payload_binary, ?array $options = null): string */
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_Quicpro_MCP_request, 0, 3, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, service_name, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, method_name, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, request_payload_binary, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, options, IS_ARRAY, 1)
ZEND_END_ARG_INFO()
/* }}} */

/* {{{ Quicpro\MCP::uploadStream(string $service, string $method, string $stream_id, resource $stream): bool */
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_Quicpro_MCP_uploadStream, 0, 4, _IS_BOOL, 0)
    ZEND_ARG_TYPE_INFO(0, service_name, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, method_name, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, stream_identifier, IS_STRING, 0)
    ZEND_ARG_INFO(0, php_stream_resource) /* resource */
ZEND_END_ARG_INFO()
/* }}} */

/* {{{ Quicpro\MCP::downloadStream(string $service, string $method, string $request_payload, resource $stream): bool */
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_Quicpro_MCP_downloadStream, 0, 4, _IS_BOOL, 0)
    ZEND_ARG_TYPE_INFO(0, service_name, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, method_name, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, request_payload_binary, IS_STRING, 0)
    ZEND_ARG_INFO(0, php_writable_stream_resource) /* resource */
ZEND_END_ARG_INFO()
/* }}} */


/* ============================================================================== */
/* == Quicpro\IIBIN Class (Static)                                             == */
/* ============================================================================== */

/* {{{ Quicpro\IIBIN::defineEnum(string $enumName, array $enumValues): bool */
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_Quicpro_IIBIN_defineEnum, 0, 2, _IS_BOOL, 0)
    ZEND_ARG_TYPE_INFO(0, enumName, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, enumValues, IS_ARRAY, 0)
ZEND_END_ARG_INFO()
/* }}} */

/* {{{ Quicpro\IIBIN::defineSchema(string $schemaName, array $schemaDefinition): bool */
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_Quicpro_IIBIN_defineSchema, 0, 2, _IS_BOOL, 0)
    ZEND_ARG_TYPE_INFO(0, schemaName, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, schemaDefinition, IS_ARRAY, 0)
ZEND_END_ARG_INFO()
/* }}} */

/* {{{ Quicpro\IIBIN::encode(string $schemaName, array|object $phpData): string */
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_Quicpro_IIBIN_encode, 0, 2, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, schemaName, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, phpData, 0, 0)
ZEND_END_ARG_INFO()
/* }}} */

/* {{{ Quicpro\IIBIN::decode(string $schemaName, string $binaryData, bool $asObject = false): array|object */
ZEND_BEGIN_ARG_INFO_EX(arginfo_Quicpro_IIBIN_decode, 0, 0, 2)
    ZEND_ARG_TYPE_INFO(0, schemaName, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, binaryData, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, asObject, _IS_BOOL, 1)
ZEND_END_ARG_INFO()
/* }}} */

/* {{{ Quicpro\IIBIN::isDefined(string $name): bool */
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_Quicpro_IIBIN_isDefined, 0, 1, _IS_BOOL, 0)
    ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
ZEND_END_ARG_INFO()
/* }}} */


/* ============================================================================== */
/* == Quicpro\Cluster Class (Static)                                           == */
/* ============================================================================== */

/* {{{ Quicpro\Cluster::orchestrate(array $options): bool */
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_Quicpro_Cluster_orchestrate, 0, 1, _IS_BOOL, 0)
    ZEND_ARG_TYPE_INFO(0, options, IS_ARRAY, 0)
ZEND_END_ARG_INFO()
/* }}} */

/* {{{ Quicpro\Cluster::signalWorkers(int $signal, string $pidFile): bool */
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_Quicpro_Cluster_signalWorkers, 0, 2, _IS_BOOL, 0)
    ZEND_ARG_TYPE_INFO(0, signal, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(0, pidFile, IS_STRING, 0)
ZEND_END_ARG_INFO()
/* }}} */

/* {{{ Quicpro\Cluster::getStats(string $pidFile): array */
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_Quicpro_Cluster_getStats, 0, 1, IS_ARRAY, 0)
    ZEND_ARG_TYPE_INFO(0, pidFile, IS_STRING, 0)
ZEND_END_ARG_INFO()
/* }}} */


/* ============================================================================== */
/* == Quicpro\PipelineOrchestrator Class (Static)                              == */
/* ============================================================================== */

/* {{{ Quicpro\PipelineOrchestrator::run(mixed $initialData, array $pipelineDefinition, ?array $options = null): object */
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_Quicpro_PipelineOrchestrator_run, 0, 2, IS_OBJECT, 0)
    ZEND_ARG_INFO(0, initialData)
    ZEND_ARG_TYPE_INFO(0, pipelineDefinition, IS_ARRAY, 0)
    ZEND_ARG_TYPE_INFO(0, executionOptions, IS_ARRAY, 1)
ZEND_END_ARG_INFO()
/* }}} */

/* {{{ Quicpro\PipelineOrchestrator::registerToolHandler(string $toolName, array $handlerConfig): bool */
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_Quicpro_PipelineOrchestrator_registerToolHandler, 0, 2, _IS_BOOL, 0)
    ZEND_ARG_TYPE_INFO(0, toolName, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, handlerConfiguration, IS_ARRAY, 0)
ZEND_END_ARG_INFO()
/* }}} */


/* ============================================================================== */
/* == Quicpro\WebSocket Class                                                  == */
/* ============================================================================== */

/* {{{ Quicpro\WebSocket::connect(string $host, int $port, string $path, ?array $headers = null, ?resource $config = null): object */
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_Quicpro_WebSocket_connect, 0, 3, IS_OBJECT, 0)
    ZEND_ARG_TYPE_INFO(0, host,    IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, port,    IS_LONG,   0)
    ZEND_ARG_TYPE_INFO(0, path,    IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, headers, IS_ARRAY,  1)
    ZEND_ARG_INFO(0, config) /* resource, optional */
ZEND_END_ARG_INFO()
/* }}} */

/* {{{ Quicpro\MCP::upgradeToWebSocket(string $path, ?array $headers = null): object */
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_Quicpro_MCP_upgradeToWebSocket, 0, 1, IS_OBJECT, 0)
    ZEND_ARG_TYPE_INFO(0, path,    IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, headers, IS_ARRAY,  1)
ZEND_END_ARG_INFO()
/* }}} */

/* {{{ Quicpro\WebSocket::send(string $data, bool $is_binary = false): bool */
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_Quicpro_WebSocket_send, 0, 1, _IS_BOOL, 0)
    ZEND_ARG_TYPE_INFO(0, data, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, is_binary, _IS_BOOL, 1)
ZEND_END_ARG_INFO()
/* }}} */

/* {{{ Quicpro\WebSocket::receive(?int $timeout_ms = -1): ?string */
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_Quicpro_WebSocket_receive, 0, 0, IS_STRING, 1)
    ZEND_ARG_TYPE_INFO(0, timeout_ms, IS_LONG, 1)
ZEND_END_ARG_INFO()
/* }}} */

/* {{{ Quicpro\WebSocket::close(): void */
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_Quicpro_WebSocket_close, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()
/* }}} */

/* {{{ Quicpro\WebSocket::getStatus(): int */
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_Quicpro_WebSocket_getStatus, 0, 0, IS_LONG, 0)
ZEND_END_ARG_INFO()
/* }}} */


/* Note: `quicpro_cancel_stream` and other low-level functions might be deprecated
 * in favor of methods on the connection objects, or kept as internal helpers.
 * For now, it is kept as an example of a global-space function.
 */
ZEND_BEGIN_ARG_INFO_EX(arginfo_quicpro_cancel_stream_global, 0, 0, 2)
    ZEND_ARG_INFO(0, mcp_session_resource, 0)
    ZEND_ARG_TYPE_INFO(0, stream_id, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(0, how, IS_STRING, 1)
ZEND_END_ARG_INFO()


#endif /* PHP_QUICPRO_ARGINFO_H */
