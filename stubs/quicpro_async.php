<?php

/**
 * Quicpro Async Stub File
 * =========================================================================
 *
 * This file provides a user-land façade for the `quicpro_async` PECL extension.
 * Its purpose is to enable static analysis tools (Psalm, PHPStan) and IDEs
 * (like PhpStorm) to understand the extension's classes, methods, and constants
 * without needing to parse the C source code.
 *
 * All method bodies are empty stubs; the actual logic is implemented in the
 * native C extension.
 */

declare(strict_types=1);

namespace Quicpro {

    // --- Constants ---

    // For Quicpro\Cluster scheduler policy
    define('Quicpro\QUICPRO_SCHED_OTHER', 0);
    define('Quicpro\QUICPRO_SCHED_FIFO', 1);
    define('Quicpro\QUICPRO_SCHED_RR', 2);

    // For `quicpro.session_mode` INI setting and Config option
    define('Quicpro\QUICPRO_SESSION_AUTO', 0);
    define('Quicpro\QUICPRO_SESSION_LIVE', 1);
    define('Quicpro\QUICPRO_SESSION_TICKET', 2);
    define('Quicpro\QUICPRO_SESSION_WARM', 3);
    define('Quicpro\QUICPRO_SESSION_SHM_LRU', 4);

    /**
     * An immutable configuration object used to define the behavior of a QUIC session.
     */
    final class Config
    {
        /**
         * Creates a new, immutable configuration object.
         *
         * @param array $options An associative array of configuration parameters.
         * @throws \Quicpro\Exception\PolicyViolationException If overriding php.ini is disallowed.
         * @throws \Quicpro\Exception\UnknownConfigOptionException For invalid option keys.
         * @throws \Quicpro\Exception\InvalidConfigValueException For values of the wrong type.
         */
        public static function new(array $options = []): self
        {
            // C-level implementation
            return new self;
        }
    }

    /**
     * Represents a single, active QUIC connection, handling the underlying socket,
     * TLS state, and HTTP/3 communication.
     */
    final class Session
    {
        /**
         * Establishes a new QUIC session and performs the TLS 1.3 handshake.
         */
        public function __construct(string $host, int $port, Config $config)
        {
            // C-level implementation
        }

        /**
         * Checks if the QUIC handshake has successfully completed and the session is active.
         */
        public function isConnected(): bool
        {
            // C-level implementation
            return false;
        }

        /**
         * Sends an HTTP/3 request on a new stream.
         *
         * @param array<string, string> $headers
         */
        public function sendRequest(string $method, string $path, array $headers = [], ?string $body = null): int
        {
            // C-level implementation
            return 0;
        }

        /**
         * Reads the next available event from a specific stream.
         *
         * @return array{type: 'headers'|'body_chunk'|'finished', data: mixed}|null
         */
        public function readStream(int $streamId): ?array
        {
            // C-level implementation
            return null;
        }

        /**
         * Drives the connection's event loop, processing incoming/outgoing packets and timers.
         * This must be called periodically in your application's event loop.
         */
        public function poll(int $timeout_ms): void
        {
            // C-level implementation
        }

        /**
         * Gracefully closes the QUIC connection, sending a CONNECTION_CLOSE frame.
         */
        public function close(): void
        {
            // C-level implementation
        }

        // ... other Session helper methods like getStats, exportTicket etc.
    }

    /**
     * Represents a client for the Model Context Protocol (MCP).
     */
    final class MCP
    {
        public function __construct(string $host, int $port, Config $config)
        {
            // C-level implementation
        }

        /**
         * Sends a unary (request-response) RPC to an MCP service.
         *
         * @param string $request_payload_binary A binary payload created with Quicpro\IIBIN::encode().
         * @return string The binary response payload, to be decoded with Quicpro\IIBIN::decode().
         */
        public function request(string $service_name, string $method_name, string $request_payload_binary, array $options = []): string
        {
            // C-level implementation
            return '';
        }

        public function close(): void
        {
            // C-level implementation
        }
    }

    /**
     * Represents a WebSocket over HTTP/3 connection.
     */
    final class WebSocket
    {
        public static function connect(string $host, int $port, string $path, array $headers = [], ?Config $config = null): self
        {
            // C-level implementation
            return new self;
        }

        public function send(string $data, bool $is_binary = false): bool
        {
            // C-level implementation
            return false;
        }

        public function receive(int $timeout_ms = -1): ?string
        {
            // C-level implementation
            return null;
        }

        public function close(): void
        {
            // C-level implementation
        }
    }

    /**
     * A static class for the C-native "Intelligent Intern Binary" (IIBIN) serialization.
     */
    final class IIBIN
    {
        public static function defineSchema(string $schemaName, array $schemaDefinition): bool
        {
            // C-level implementation
            return false;
        }

        public static function defineEnum(string $enumName, array $enumValues): bool
        {
            // C-level implementation
            return false;
        }

        /**
         * @param array|object $phpData
         */
        public static function encode(string $schemaName, $phpData): string
        {
            // C-level implementation
            return '';
        }

        /**
         * @return array|object
         */
        public static function decode(string $schemaName, string $binaryData, bool $asObject = false)
        {
            // C-level implementation
            return [];
        }
    }

    /**
     * Static class for the C-native Pipeline Orchestrator engine.
     */
    final class PipelineOrchestrator
    {
        /**
         * @param mixed $initialData
         */
        public static function run($initialData, array $pipelineDefinition, array $options = []): object
        {
            // C-level implementation
            return new \stdClass;
        }

        public static function registerToolHandler(string $toolName, array $handlerConfig): bool
        {
            // C-level implementation
            return false;
        }

        public static function enableAutoContextLogging(array $loggerConfig): bool
        {
            // C-level implementation
            return false;
        }
    }

    /**
     * Static class for the C-native multi-process supervisor.
     */
    final class Cluster
    {
        /**
         * Starts the blocking supervisor loop. Forks workers that execute the provided callable.
         */
        public static function orchestrate(array $options): bool
        {
            // C-level implementation
            return false;
        }
    }

    /**
     * A simple server implementation for listening for incoming QUIC connections.
     */
    final class Server
    {
        public function __construct(string $host, int $port, Config $config)
        {
            // C-level implementation
        }

        public function accept(): ?Session
        {
            // C-level implementation
            return null;
        }
    }
}

namespace Quicpro\Exception {

    class QuicproException extends \RuntimeException {}

    // Configuration Layer
    class ConfigException extends QuicproException {}
    class PolicyViolationException extends ConfigException {}
    class UnknownConfigOptionException extends ConfigException {}
    class InvalidConfigValueException extends ConfigException {}

    // Transport & Connection Layer
    class ConnectionException extends QuicproException {}
    class TlsHandshakeException extends ConnectionException {}
    class ProtocolViolationException extends ConnectionException {}
    class TimeoutException extends ConnectionException {}
    class HandshakeTimeoutException extends TimeoutException {}
    class IdleTimeoutException extends TimeoutException {}
    class ResponseTimeoutException extends TimeoutException {}

    // Stream & Frame Layer
    class StreamException extends QuicproException {}
    class StreamLimitException extends StreamException {}
    class FrameException extends StreamException {}

    // High-Level API Layer
    class PipelineException extends QuicproException {}
    class IIBINException extends QuicproException {}
    class MCPException extends QuicproException {}
    class WebSocketException extends QuicproException {}

    // Server & Cluster Layer
    class ServerException extends QuicproException {}
    class ClusterException extends QuicproException {}
}