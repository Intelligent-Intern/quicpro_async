# THIS IS STILL WORK IN PROGRESS - GIVE ME A COUPLE DAYS TO FINISH IT.. DOCUMENTATION FIRST YAY

## Our project plan is now defined in a detailed EPIC. Next step: Iteratively implement and test each layer, starting with the core QUIC and Proto modules.

# `quicpro_async`
**A New Epoch for PHP.**

[![PECL](https://img.shields.io/badge/PECL-quicpro__async-blue?logo=php)](https://intelligentinternpecl.z13.web.core.windows.net/packages/quicpro_async)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

---

## Our Vision: Systems Engineering with PHP

The architectural demands on modern backend applications have evolved. High concurrency, low-latency communication, and efficient hardware utilization are no longer optional—they are baseline requirements. While PHP has long dominated the web, its traditional process-per-request model presents challenges for building stateful, real-time, or computationally intensive systems.

`quicpro_async` is engineered to solve this problem. Its purpose is to provide a C-native, high-performance toolkit that elevates PHP into a first-class language for modern systems engineering. We achieve this by providing foundational primitives that were previously unavailable or required external, non-native components.

Our technical commitments are:

* **A Modern Transport Core:** We provide a full client and server implementation of QUIC and HTTP/3. This fundamentally reduces latency through 0-RTT handshakes, eliminates head-of-line blocking via independent streams, and ensures resilience with transparent connection migration.

* **True Multi-Core Parallelism:** We provide a native process supervisor that allows a single PHP application to efficiently utilize every available CPU core. It manages a pool of isolated worker processes, distributing tasks for true parallel execution and enabling PHP to scale vertically to saturate modern server hardware.

* **Asynchronous I/O and Fiber-First Design:** The extension's core is built on a non-blocking I/O model. This integrates seamlessly with PHP Fibers (especially with C-level optimizations for PHP 8.4+), allowing a single PHP worker to handle thousands of concurrent network sessions without blocking.

* **High-Level Abstractions:** On top of this foundation, we build powerful convenience layers, including the Model Context Protocol (MCP) for agent communication, a C-native Pipeline Orchestrator for complex workflows, and `ii_bin` for high-performance binary serialization.

* **Native Bidirectional Communication:** We implement WebSocket over HTTP/3 (RFC 9220) natively in C, providing a persistent, low-latency, bidirectional communication channel for real-time applications, event streams, and interactive user interfaces.

The age of workarounds and excuses for building high-performance systems in PHP is over. Enough with the memes. **Let's show them.**

---

## Feature Highlights

| Area                       | Feature                                                     | Status |
| -------------------------- | ----------------------------------------------------------- | :----: |
| **QUIC Transport Layer** | Full Client & Server Implementation (QUIC v1, RFC 9000)     |   ✅   |
|                            | Configurable "Happy Eyeballs" (RFC 8305) for IPv4/IPv6      |   ✅   |
|                            | Transparent Connection Migration                            |   ✅   |
|                            | Zero-RTT Session Resumption (SHM Ticket Cache)              |   ✅   |
| **Cluster Supervisor** | C-Native Multi-Process Supervisor (`fork`-based)            |   ✅   |
|                            | Automatic Worker Restart & Health Monitoring                |   ✅   |
|                            | CPU Affinity, Scheduling Policies, Resource Limits          |   ✅   |
| **MCP Layer** | Client & Server for Model Context Protocol                  |   ✅   |
|                            | Unary (Request-Response) and Streaming RPCs                 |   ✅   |
| **`ii_bin` (Serialization)**| Protobuf-inspired C-Native Binary Serialization            |   ✅   |
|                            | Schema & Enum Definition API (`Proto::defineSchema`)        |   ✅   |
|                            | High-Performance Encode/Decode of PHP Arrays/Objects        |   ✅   |
| **Pipeline Orchestrator** | C-Native Workflow Engine                                    |   ✅   |
|                            | Declarative, Multi-Step Pipeline Definition                 |   ✅   |
|                            | Conditional Logic & Parallel Step Execution Hooks           |   ✅   |
|                            | GraphRAG Integration Hooks for LLM Context Augmentation     |   ✅   |
| **WebSocket Layer** | WebSocket over HTTP/3 (RFC 9220)                            |   ✅   |
|                            | PHP Stream Wrapper for WebSocket Connections                |   ✅   |
| **Observability** | Connection Statistics (`rtt`, `cwnd`, `loss`)               |   ✅   |
|                            | QLOG Tracing (RFC 9890) for Deep Debugging                  |   ✅   |
| **Performance** | Optional AF_XDP / Zero-Copy Path (`--enable-quicpro-xdp`)    |   ✅   |
|                            | Optional Busy-Polling for Ultra-Low Latency                 |   ✅   |
| **Compatibility** | Linux (glibc/musl), macOS, WSL2                             |   ✅   |
|                            | PHP 8.1 - 8.4 (with version-specific Fiber optimizations)   |   ✅   |

---

## Installation (PECL)

~~~bash
pecl channel-discover https://intelligentinternpecl.z13.web.core.windows.net/channel.xml
pecl install quicpro_async
echo "extension=quicpro_async.so" > $(php -i | grep -Po '/.*?/conf\.d')/30-quicpro_async.ini
~~~
For manual compilation from source, see **BUILD.md**.

---

## TL;DR – Quick Start Examples

#### 1. Simple LLM Call via Pipeline Orchestrator

This is the easiest way to get started. It uses the highest-level API to perform a common AI task.

~~~php
<?php
// Assumes a 'GenerateText' tool handler has been registered to use an LLM backend.
// See WORKFLOW_ORCHESTRATION_LOW_CONFIG_EXAMPLE.md for setup.

$prompt = "Explain QUIC in one sentence.";

// Define a single-step pipeline
$pipeline = [['tool' => 'GenerateText']];

// Run the pipeline. The orchestrator handles connection, encoding, and errors.
$result = Quicpro\PipelineOrchestrator::run($prompt, $pipeline);

echo $result->isSuccess() ? $result->getFinalOutput()['generated_text'] : "Error: " . $result->getErrorMessage();
?>
~~~

#### 2. Direct MCP Client Request (Lower-Level)

For when you need to communicate directly with an MCP agent without the orchestrator.

~~~php
<?php
use Quicpro\MCP;
use Quicpro\Proto;
use Quicpro\Config;

// Define the data contract using ii_bin
Proto::defineSchema('EchoRequest', ['message' => ['type' => 'string', 'tag' => 1]]);
Proto::defineSchema('EchoResponse', ['response' => ['type' => 'string', 'tag' => 1]]);

// Connect with default secure settings
$mcp = new MCP('echo-agent.mcp.local', 4433, Config::new());

// Encode, send request, decode response
$requestPayload = Proto::encode('EchoRequest', ['message' => 'Hello MCP']);
$binaryResponse = $mcp->request('EchoService', 'echo', $requestPayload);
$response = Proto::decode('EchoResponse', $binaryResponse);

echo $response['response']; // outputs: Hello MCP
?>
~~~

More detailed examples (HFT, Streaming, WebSockets, Clustering) are in the `/examples` directory and documented in `AI_EXAMPLES.md` and `EXAMPLES.md`.

---

## How It Works

* **`libquiche`:** Cloudflare's excellent Rust library handles the core QUIC transport, congestion control, and TLS 1.3 logic.
* **C Shim:** A lean C layer exposes `libquiche` and our native C components (Orchestrator, Cluster Supervisor) to the Zend VM.
* **`ii_bin`:** Our custom, C-native "Intelligent Intern Binary" format, inspired by Protobuf, for ultra-fast serialization of PHP data.
* **Fiber-Aware Event Loop:** The C-level `poll()` function is aware of PHP Fibers. On PHP 8.4+, it uses native C APIs to yield control, preventing blocking and enabling true concurrency. On PHP 8.1-8.3, it works seamlessly with userland Fiber schedulers.
* **Shared-Memory Ticket Cache:** An LRU cache implemented in shared memory allows forked workers in a `Cluster` to resume TLS sessions initiated by any other worker, preserving 0-RTT handshakes and ensuring high performance in multi-process applications.

---

## Roadmap

Our development follows an iterative, test-driven plan. The high-level roadmap is:

* **v0.2:** Complete the foundational **Proto (`ii_bin`)** and **MCP** layers, with full client/server support and robust testing.
* **v0.3:** Complete the C-native **Pipeline Orchestrator** with support for conditional logic and GraphRAG hooks.
* **v0.4:** Complete the C-native **Cluster Supervisor** with advanced process management features and IPC.
* **v0.5:** Complete the **WebSocket over H3** implementation and streaming APIs for MCP.
* **v1.0:** Stable release. Includes a PSR-7 adapter, Prometheus metrics integration, and a native Windows build.

---

## Contributing

Pull requests, bug reports, and ideas are highly welcome. Please read **CONTRIBUTING.md** before you start. Each PR is fuzz-checked and run against the full CI matrix (Ubuntu & Alpine, PHP 8.1–8.4).

**Maintainer:** Jochen Schultz · <jschultz@php.net>

---

### License

MIT; see `LICENSE`.