<?php
/**
 * Quicpro\Config: Expert Configuration Example
 * =========================================================================
 * FILENAME: docs/config/examples/expert_ai_training_cpu_pinning.php
 *
 * This script demonstrates an advanced configuration for a highly specific,
 * performance-critical use case: a parameter server in a distributed AI
 * training cluster running on modern hybrid CPUs.
 */
declare(strict_types=1);

// Import the necessary classes from the Quicpro extension.
use Quicpro\Config;
use Quicpro\MCP;
use Quicpro\Exception\QuicproException;

// --- SCENARIO: A "Parameter Server" on a Hybrid CPU (Intel P-core/E-core) ---

/**
 * This server runs on a modern Intel CPU with both Performance-cores (P-cores) and
 * Efficient-cores (E-cores). The goal is to optimize network I/O by pinning
 * different types of connections to the appropriate core type, ensuring
 * that critical path operations are never delayed by background chatter.
 *
 * ---
 *
 * ARCHITECTURAL JUSTIFICATION: Why use QUIC in a Datacenter?
 *
 * While raw TCP or RDMA might offer lower absolute overhead in a pure HPC environment,
 * using the `Quicpro` framework internally provides a unified API for observability,
 * security (via optional TLS), and high-level abstractions like MCP for both
 * internal (East-West) and external (North-South) communication. This example
 * prioritizes a consistent development ecosystem over absolute minimal protocol overhead.
 */

// This part of the script would be the main logic of your parameter server.
try {
    // Configuration for critical, low-latency communication with training workers.
    // We hint that the I/O threads for these connections should run on P-cores.
    $pCoreConfig = Config::new([
        'tls_enable' => false, // Trusted datacenter network.
        'alpn'       => ['mcp-gradients/1.0'],

        // A value of 0 enables a true busy-loop (spin-wait), consuming 100%
        // of the I/O thread's CPU core to achieve the absolute minimum latency.
        'busy_poll_duration_us' => 0,

        /**
         * EXPERT: Pin the I/O threads for these connections to specific P-cores.
         * The values below are illustrative for an example Intel Core i9.
         *
         * NOTE ON CPU AFFINITY:
         * The actual core IDs for P-cores and E-cores vary by CPU model. Before setting
         * affinity, you must identify the correct core ranges for your hardware.
         * - On Linux: Use the command `lscpu -e` or check `/proc/cpuinfo`.
         * - On Windows: Use Task Manager (Performance tab -> CPU, right-click graph ->
         * Change graph to -> Logical processors) or PowerShell (`Get-ComputerInfo`).
         */
        'io_thread_cpu_affinity' => [0, 2, 4, 6, 8, 10, 12, 14],
    ]);

    // Configuration for background, high-throughput tasks.
    // We hint that I/O for these connections should be handled by the E-cores.
    $eCoreConfig = Config::new([
        'tls_enable'                => false,
        'alpn'                      => ['mcp-telemetry/1.0'],
        'initial_max_data'          => 16777216, // 16MB connection window
        'initial_max_stream_data_uni' => 8388608,  // 8MB for unidirectional logging streams

        // EXPERT: Pin the I/O threads to a range of E-cores.
        'io_thread_cpu_affinity'    => [16, 17, 18, 19, 20, 21, 22, 23],
    ]);

    echo "PARAMETER SERVER: Initializing connections with CPU core pinning strategy...\n";

    // Establish high-priority connections to two training workers, handled by P-cores.
    $worker1_link = new MCP('10.0.1.10', 9000, $pCoreConfig);
    $worker2_link = new MCP('10.0.1.11', 9000, $pCoreConfig);
    echo "  ✅ MCP links to Training Workers 1 & 2 established on P-cores.\n";

    // Establish low-priority connection to a central telemetry server, handled by E-cores.
    $telemetry_link = new MCP('10.0.2.5', 8080, $eCoreConfig);
    echo "  ✅ MCP link to Telemetry Server established on E-cores.\n";

    // The main application would now enter its processing loop, using these distinct
    // connection objects to communicate. The extension's I/O threads would be
    // scheduled on the designated core types.

} catch (QuicproException $e) {
    die("Fatal Error: " . $e->getMessage() . "\n");
}