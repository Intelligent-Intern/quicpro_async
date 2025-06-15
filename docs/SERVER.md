## The `quicpro_async` Server: An Engine for Concurrent Applications

### 1. Introduction: Beyond a Simple Socket Listener

The `Quicpro\Server` class and its derivatives are the foundation for building high-performance, long-running network services in PHP. This is not a simple socket wrapper; it is a complete, C-native application server engine designed to be the foundation for building microservices in PHP. Its core responsibilities are to abstract away the immense complexity of network I/O and concurrency management, providing the developer with a clean, high-level API.

The implication of this design is profound: for many modern, high-throughput applications, the requirement for a separate, traditional web server like Nginx or Apache is rendered obsolete. The `quicpro_async` framework empowers a PHP application to **become** its own high-performance, secure web server. This power is delivered to the PHP runtime via this PECL extension, which integrates a native C/Rust core.

This is the evolution beyond the built-in development server. This is the end of the line for `php -S`.

### 2. The Core Architecture: An Event-Driven, Fiber-Per-Client Model

The recommended way to build applications is using the high-level, event-driven API. This pattern combines maximum performance with clean, maintainable application code.

* **The Event-Driven API:** Instead of writing a manual `while` loop to accept connections and read from sockets, you simply register PHP callbacks (Closures or functions) for specific server events, primarily:
  * `on('connect', function(Connection $conn) { ... })`: Fired when a new client successfully establishes a connection.
  * `on('message', function(Connection $conn, Message $msg) { ... })`: Fired when a data message is received from a client.
  * `on('close', function(Connection $conn) { ... })`: Fired when a client disconnects.

* **The C-Native Event Loop:** When you call the `$server->run()` method, you are starting a highly optimized event loop written entirely in C. This loop uses the most efficient I/O notification system available on the host OS (like `epoll` on Linux) to monitor all active network sockets simultaneously.

* **Automatic Concurrency with Fibers:** The `Server` engine implements a **Fiber-per-Client** concurrency model. When a new client connection is accepted, the server automatically spawns a new PHP `Fiber` to handle that specific client's lifecycle. This means that work done for one client (e.g., a slow database query) can never block the processing of requests for thousands of other clients. This enables true, massive concurrency within a single PHP process.

### 3. Centralized Policy Enforcement at the Edge

Because the `Server` class is the single entry point for all incoming connections, it is the perfect place to enforce cross-cutting security policies. This logic is handled by the C-core *before* your PHP application code is ever executed, providing maximum performance and security.

* **Native CORS Handling:** Instead of checking CORS headers in your PHP code, you define the policy in the `Config` object passed to the `Server`. The C-native server core then automatically inspects incoming requests, handles `OPTIONS` preflights, and adds the necessary headers for requests from permitted domains. Your PHP code only ever sees requests that have already passed the CORS policy check.

* **Native Rate Limiting:** Similarly, the high-performance rate limiting module is hooked into the server's connection acceptance logic. The C-core checks each incoming connection against the shared-memory-based rate limiter. If a client has exceeded its request limit, its connection attempt is dropped at the lowest possible level, protecting the application from being flooded.

### 4. A Unified Platform for Diverse Protocols

The `Quicpro\Server` class provides the fundamental building blocks (`accept()`, `poll()`, etc.) upon which more specialized, high-level servers are built. This allows `quicpro_async` to act as a universal platform for any network protocol:

* **`Quicpro\WebSocket\Server`:** A specialized server class for handling real-time browser communication. Its `on('message', ...)` event directly provides clean, de-framed WebSocket messages.
* **`Quicpro\MCP\Server`:** The foundation for our internal microservices. It listens for incoming `IIBIN`-encoded requests and routes them to registered service handlers.
* **Custom Protocol Servers:** The core primitives allow you to build servers for any protocol, from industrial standards like Modbus and OPC UA to a custom "Smart DNS" server listening on UDP port 53.

## 5. The Server in Practice: An Event-Driven WebSocket Example

This section translates the architectural concepts from the introduction into a concrete, runnable example. We will build a simple, high-performance WebSocket echo server. This example exclusively uses the high-level, event-driven API, which is the recommended and most elegant way to build applications with the framework. It highlights how the C-native core handles all complex I/O and concurrency, leaving the developer with clean, readable PHP code that focuses purely on the application's business logic.

### Server Lifecycle & Concurrency Model

The following diagram visualizes what happens "under the hood" when you call the `$server->run()` method. It illustrates the C-native event loop and the "Fiber-per-Client" model, which is the key to the server's ability to handle massive concurrency without blocking.

~~~mermaid
graph TD
    subgraph "PHP Userland"
        A["PHP Script calls `$server->run()`"];
        F["on('connect') callback executes"];
        H["on('message') callback executes"];
        J["on('close') callback executes"];
    end

    subgraph "quicpro_async C-Engine"
        B["C-Native Event Loop starts<br/>(e.g., epoll_wait)<br/> - "];
        C{"New incoming QUIC connection?"};
        D["C-Engine accepts connection"];
        E["Spawns a new PHP Fiber<br/>(for this specific client)<br/> - "];
        G["C-Engine receives WebSocket message"];
        I["Client connection closes"];
        K("Fiber terminates");
    end
    
    A --> B;
    B --> C;
    C -- Yes --> D;
    D --> E;
    E --> F;
    F --> G;
    C -- No --> B;
    G -- Message received --> H;
    H --> G;
    G -- Disconnect event --> I;
    I --> J;
    J --> K;

    classDef php fill:#e0f7fa,stroke:#00796b,stroke-width:2px,color:#004d40
    classDef c fill:#f3e5f5,stroke:#512da8,stroke-width:2px,color:#311b92
    class A,F,H,J php
    class B,C,D,E,G,I,K c
~~~

### Example: A Concurrent WebSocket Echo Server

The following code is a complete, standalone WebSocket server. It listens for connections, and for each connected client, it simply echoes back any message it receives. Thanks to the underlying C-engine and Fiber-per-client model, this simple script can handle thousands of concurrent echo sessions.

~~~php
<?php
// FILENAME: websocket_echo_server.php
declare(strict_types=1);

// This script requires the Quicpro PECL extension.
if (!extension_loaded('quicpro_async')) {
    die("Fatal Error: The 'quicpro_async' extension is not loaded. Please check your php.ini.\n");
}

use Quicpro\Config;
use Quicpro\WebSocket\Server as WebSocketServer;
use Quicpro\WebSocket\Connection;
use Quicpro\Exception\QuicproException;

try {
    // 1. Create a server configuration with TLS certificates, which are
    // mandatory for secure WebSockets (wss://).
    $serverConfig = Config::new([
        'cert_file' => './certs/server.pem',
        'key_file'  => './certs/server.key',
        'alpn'      => ['h3'], // WebSocket upgrade requires an HTTP/3 handshake.
    ]);

    // 2. Instantiate the high-level WebSocket server.
    $wsServer = new WebSocketServer('0.0.0.0', 4433, $serverConfig);

    /**
     * 3. Register Event Handlers.
     * This is the core of the application logic. We provide closures that
     * the C-level event loop will execute when specific events occur.
     */

    // The 'connect' event is fired after a client's QUIC and WebSocket
    // handshakes are successfully completed.
    $wsServer->on('connect', function (Connection $connection) {
        echo "[INFO] Client #{$connection->getId()} connected from {$connection->getRemoteAddress()}.\n";
        $connection->send("Welcome to the quicpro_async Echo Server!");
    });

    // The 'message' event is fired whenever a complete data message
    // is received from a client. The C-core handles all the low-level
    // frame parsing and reassembly for fragmented messages.
    $wsServer->on('message', function (Connection $connection, string $message) {
        echo "[DATA] Received from client #{$connection->getId()}: {$message}\n";

        // The entire application logic is this single line.
        $connection->send("ECHO: " . $message);
    });

    // The 'close' event is fired when a client disconnects or the
    // connection is terminated.
    $wsServer->on('close', function (Connection $connection) {
        echo "[INFO] Client #{$connection->getId()} disconnected.\n";
    });

    echo "High-Performance WebSocket Echo Server listening on udp://0.0.0.0:4433...\n";
    
    // 4. Start the server's event loop.
    // This `run()` method is a blocking call that starts the highly-optimized,
    // C-native event loop, which handles all I/O, polling, and concurrency.
    $wsServer->run();

} catch (QuicproException | \Exception $e) {
    die("A fatal server error occurred: " . $e->getMessage() . "\n");
}
~~~

## 6. Building a Secure Filesystem Agent with MCP

This section details how to build another type of specialized server: a secure, sandboxed filesystem agent. This demonstrates how the `quicpro_async` framework can be used to create robust microservices that encapsulate and secure access to critical resources like the file system.

### The Architectural Pattern: The Filesystem Gateway

In a complex application, allowing your main business logic services to have direct, unrestricted access to the filesystem is a significant security risk. A vulnerability in one service could potentially expose the entire server's file system. Furthermore, managing file access permissions and business rules across a distributed cluster of application servers is a complex and error-prone task.

The solution is to create a dedicated **Filesystem Agent**. This is a standalone MCP server whose sole responsibility is to provide a controlled and secure API for file operations within a specific, "sandboxed" directory.

* **Enhanced Security:** The agent runs as a separate process, potentially under a less-privileged user account. Its most important job is to validate all incoming path requests to prevent **path traversal attacks**, ensuring that no client can ever access a file outside of its designated root directory (e.g., preventing access to `/etc/passwd`).
* **Centralized Logic:** All business rules related to file access (e.g., naming conventions, access control checks, logging) are centralized in this one service, making them easy to audit and maintain.
* **Abstraction:** Other services in your ecosystem no longer need to know about physical file paths. They simply make a clean, high-level MCP call, for example: `Filesystem->readFile({path: 'invoices/2024/inv_123.pdf'})`.

### Architectural Diagram

This diagram shows how an application service (like a PDF report generator) communicates with the Filesystem Agent via MCP, without ever directly touching the disk itself.

~~~mermaid
graph LR
    subgraph "Application Layer"
        A["fa:fa-cogs Application Agent<br/>(e.g., Report Generator)<br/> - "];
    end

    subgraph "Filesystem Gateway (This Server)"
        B["fa:fa-shield-alt Filesystem Agent<br/>(PHP + MCP Server)<br/> - "];
    end

    subgraph "Physical Resource"
        C("fa:fa-folder-open Secure Sandbox<br/>'/var/www/uploads'<br/> - ");
        D["fa:fa-file-pdf report.pdf"];
        C -- Contains --> D;
    end
    
    A -- "1. MCP Request:<br/>readFile('report.pdf')<br/> - " --> B;
    B -- "2. Validates & Sanitizes Path" --> C;
    C -- "3. Reads File" --> D;
    D -- "File Content" --> B;
    B -- "4. MCP Response<br/>(with file content)<br/> - " --> A;
    
    class A,B,C,D internal;
~~~

### Implementation: The Sandboxed Filesystem Agent

The following script is a complete, runnable MCP server that acts as a secure filesystem gateway. It uses the high-level `Quicpro\MCP\Server` API, which encapsulates the event loop and request routing.

~~~php
<?php
// FILENAME: filesystem_agent.php
declare(strict_types=1);

if (!extension_loaded('quicpro_async')) {
    die("Fatal Error: The 'quicpro_async' extension is not loaded.\n");
}

use Quicpro\Config;
use Quicpro\MCP\Server as McpServer;
use Quicpro\IIBIN;
use Quicpro\Exception\QuicproException;

// --- Agent Configuration ---
const AGENT_HOST = '127.0.0.1'; // Listen on internal interface only
const AGENT_PORT = 9901;
const SANDBOX_ROOT = __DIR__ . '/../storage/safe_uploads'; // The ONLY directory this agent can access.

/**
 * This class implements the service logic. It will be registered with the MCP
 * server, which will route incoming requests to these public methods.
 */
class FilesystemService
{
    private string $workDir;

    public function __construct(string $sandboxPath)
    {
        // On startup, resolve the absolute path to the sandbox directory.
        // This is a critical security step.
        $this->workDir = realpath($sandboxPath);
        if ($this->workDir === false) {
            // Create the directory if it doesn't exist.
            if (!mkdir($sandboxPath, 0770, true) && !is_dir($sandboxPath)) {
                 throw new \RuntimeException("Sandbox directory '{$sandboxPath}' does not exist and could not be created.");
            }
            $this->workDir = realpath($sandboxPath);
        }
        echo "Filesystem Agent sandboxed to: {$this->workDir}\n";
    }

    /**
     * A private security helper to resolve a user-provided relative path
     * and ensure it remains strictly within the sandboxed working directory.
     * Prevents all path traversal attacks (e.g., `../../etc/passwd`).
     */
    private function securePath(string $relativePath): string|false
    {
        $fullPath = realpath($this->workDir . '/' . $relativePath);
        if ($fullPath === false || strpos($fullPath, $this->workDir) !== 0) {
            return false; // The path is either invalid or outside the sandbox.
        }
        return $fullPath;
    }
    
    public function list_files(string $payload): string
    {
        $input = IIBIN::decode('PathRequest', $payload);
        $path = $this->securePath($input['path'] ?? '.');
        if ($path === false) { return IIBIN::encode('ErrorResponse', ['error' => 'Access denied or path not found.']); }
        
        $files = scandir($path);
        return IIBIN::encode('FileListResponse', ['files' => array_values(array_diff($files, ['.', '..']))]);
    }

    public function read_file(string $payload): string
    {
        $input = IIBIN::decode('PathRequest', $payload);
        $path = $this->securePath($input['path']);
        if ($path === false || !is_file($path)) {
             return IIBIN::encode('ErrorResponse', ['error' => 'File not found or access denied.']);
        }
        return IIBIN::encode('FileContentResponse', ['content' => file_get_contents($path)]);
    }

    public function write_file(string $payload): string
    {
        $input = IIBIN::decode('WriteFileRequest', $payload);
        $path = $this->securePath($input['path']);
        // For writing, we must also ensure the directory exists first.
        $dir = dirname($path);
        if ($dir === false || strpos($dir, $this->workDir) !== 0) {
            return IIBIN::encode('ErrorResponse', ['error' => 'Invalid path specified.']);
        }
        
        file_put_contents($path, $input['content']);
        return IIBIN::encode('SuccessResponse', ['status' => 'OK']);
    }
}

// --- Main Server Execution ---
try {
    // 1. Define the data contracts (schemas) for this agent's API.
    IIBIN::defineSchema('PathRequest', ['path' => ['type' => 'string', 'tag' => 1]]);
    IIBIN::defineSchema('FileListResponse', ['files' => ['type' => 'repeated_string', 'tag' => 1]]);
    IIBIN::defineSchema('FileContentResponse', ['content' => ['type' => 'bytes', 'tag' => 1]]);
    IIBIN::defineSchema('WriteFileRequest', ['path' => ['type' => 'string', 'tag' => 1], 'content' => ['type' => 'bytes', 'tag' => 2]]);
    IIBIN::defineSchema('SuccessResponse', ['status' => ['type' => 'string', 'tag' => 1]]);
    IIBIN::defineSchema('ErrorResponse', ['error' => ['type' => 'string', 'tag' => 1]]);

    // 2. Create the server configuration. For an internal agent, we would
    //    use mTLS for maximum security.
    $serverConfig = Config::new([
        'alpn' => ['mcp/1.0'],
        // 'cert_file' => './certs/filesystem_agent.pem', // Add mTLS config here
        // 'key_file' => './certs/filesystem_agent.key',
        // 'ca_file' => './ca/ca.pem',
        // 'verify_peer' => true,
    ]);
    
    // 3. Instantiate the MCP server.
    $server = new McpServer(AGENT_HOST, AGENT_PORT, $serverConfig);
    
    // 4. Instantiate our service handler and register it with the server.
    // The server will now automatically route MCP calls for the 'Filesystem'
    // service to the public methods of our $fsService object.
    $fsService = new FilesystemService(SANDBOX_ROOT);
    $server->registerService('Filesystem', $fsService);
    
    echo "Secure Filesystem Agent listening on udp://" . AGENT_HOST . ":" . AGENT_PORT . "\n";
    
    // 5. Start the server's blocking event loop.
    $server->run();

} catch (QuicproException | \Exception $e) {
    die("A fatal agent error occurred: " . $e->getMessage() . "\n");
}
~~~



# The `quicpro_async` Server: The Power of Abstraction

This final section demonstrates the ultimate power of the `quicpro_async` framework by combining all the high-level abstractions we have designed: the event-driven `MCP\Server`, the `PipelineOrchestrator`, and the concept of declarative, tool-using AI agents.

The goal is to show how an incredibly complex application, like a fully autonomous code-editing AI agent, can be implemented with **surprisingly minimal code**. This is possible because the framework's C-native core handles the difficult parts—concurrency, network I/O, protocol management—allowing the developer to focus on defining the high-level logic.

## The Use Case: A Fully Orchestrated AI Code Agent

We will build the complete "AI Code Agent" system we previously designed. An orchestrator will manage a conversation with a user, and when the AI model decides it needs to interact with the filesystem, the orchestrator will call a dedicated, secure `Filesystem Agent`.

The key takeaway is how little application "glue code" is needed when the framework provides powerful, high-level primitives.

### Architectural Diagram

This architecture remains the same: a clean separation between the "brain" (the orchestrator) and the "hands" (the tool agent).

~~~mermaid
graph LR
  classDef user fill:#ef6c00,color:white
  classDef main_agent fill:#0097a7,stroke:#006064,stroke-width:4px,color:white
  classDef tool_agent fill:#512da8,color:white,stroke-width:2px
  classDef external fill:#424242,color:white
  classDef filesystem fill:#4caf50,color:white

  subgraph "User Interface"
    A("fa:fa-user User at Terminal or Web UI");
  end

  subgraph "quicpro_async Backend"
    B["fa:fa-brain Main Orchestrator<br/>(Runs the Pipeline)<br/>-"];
    C["fa:fa-tools Filesystem Agent<br/>(MCP Server)<br/>-"];
    D["fa:fa-key LLM API Agent<br/>(MCP Server)<br/>-"];
  end

  subgraph "External & System Resources"
    E("fa:fa-robot External LLM API");
    F("fa:fa-folder-open Local Filesystem");
  end

  A -- "Provides initial prompt" --> B;
  B -- "MCP Tool Call<br/>readFile('main.php')" --> C;
  B -- "MCP Call to LLM Bridge" --> D;
  C -- "Performs OS calls" --> F;
  D -- "Secure HTTPS Call" --> E;
  E -- "LLM Response" --> D;
  D -- "MCP Response" --> B;
  C -- "Tool Result" --> B;
  B -- "Final answer" --> A;

  class A,F user;
  class B main_agent;
  class C,D tool_agent;
  class E external;
~~~

## Implementation: Minimal Code, Maximum Power

The entire system consists of two primary components: the agent that provides the tools, and the orchestrator that uses them.

### 1. The Filesystem Agent (`filesystem_agent.php`)

This is the MCP server that provides `read`, `list`, and `write` capabilities. Because we use the high-level `Quicpro\MCP\Server` API, the code is extremely clean and contains only the business logic for the tools themselves.

~~~php
<?php
// FILENAME: agents/filesystem_agent.php
declare(strict_types=1);

if (!extension_loaded('quicpro_async')) { die("Quicpro extension not loaded.\n"); }

use Quicpro\Config;
use Quicpro\MCP\Server as McpServer;
use Quicpro\IIBIN;
use Quicpro\Exception\QuicproException;

// The pure application logic for the service.
class FilesystemService
{
    private string $workDir;
    public function __construct(string $sandboxPath) { /* ... same as previous example ... */ }
    private function securePath(string $relativePath): string|false { /* ... same as previous example ... */ }
    public function list_files(string $payload): string { /* ... same as previous example ... */ }
    public function read_file(string $payload): string { /* ... same as previous example ... */ }
    public function write_file(string $payload): string { /* ... same as previous example ... */ }
}

try {
    // Schemas should be defined in a shared bootstrap file.
    // IIBIN::defineSchema(...);

    // The entire server setup is just a few lines of code.
    $serverConfig = Config::new([/* mTLS config here */]);
    $server = new McpServer('127.0.0.1', 9901, $serverConfig);
    
    $fsService = new FilesystemService(getcwd());
    $server->registerService('Filesystem', $fsService);
    
    echo "Secure Filesystem Agent listening on udp://127.0.0.1:9901...\n";
    $server->run();

} catch (QuicproException | \Exception $e) {
    die("A fatal agent error occurred: " . $e->getMessage() . "\n");
}
~~~

### 2. The Orchestrator (`run_code_agent_pipeline.php`)

This is where the power of the framework becomes truly apparent. Instead of a complex, hand-written loop in PHP, we define the agent's entire "thought process" as a **declarative pipeline**. The C-native `PipelineOrchestrator` then executes this workflow.

~~~php
<?php
// FILENAME: run_code_agent_pipeline.php
declare(strict_types=1);

if (!extension_loaded('quicpro_async')) { die("Quicpro extension not loaded.\n"); }

use Quicpro\PipelineOrchestrator;
use Quicpro\Exception\QuicproException;

// In a real app, these would be included from shared files.
// require_once __DIR__ . '/bootstrap_schemas.php';
// require_once __DIR__ . '/tool_handlers/register_all.php';

try {
    // The entire logic of the agent is defined in this data structure.
    $pipelineDefinition = [
        [
            'id' => 'InitializeState',
            'tool' => 'SetVariable', // A built-in orchestrator tool
            'params' => [
                'conversation_history' => [],
                'loop_continue' => true,
            ]
        ],
        [
            'type' => 'while',
            'condition' => '@State.loop_continue == true',
            'pipeline' => [
                [
                    'id' => 'GetUserInput',
                    'tool' => 'ReadFromCLI', // A built-in tool to get user input.
                    'params' => ['prompt' => 'You: '],
                ],
                [
                    'id' => 'AppendUserMessage',
                    'tool' => 'UpdateState', // Append the user message to the conversation history.
                    'params' => ['conversation_history' => '@State.conversation_history + {"role": "user", "content": "@GetUserInput.text"}'],
                ],
                [
                    'id' => 'CallLLM',
                    'tool' => 'AnthropicAgent', // A registered handler for the Anthropic API bridge agent.
                    'input_map' => ['messages' => '@State.conversation_history'],
                    // This tool's output would be complex, e.g., { type: 'text'|'tool_use', ... }
                ],
                [
                    'id' => 'AppendLLMResponse',
                    'tool' => 'UpdateState',
                    'params' => ['conversation_history' => '@State.conversation_history + {"role": "assistant", "content": "@CallLLM.raw_content"}'],
                ],
                [
                    'id' => 'ExecuteToolIfRequested',
                    'tool' => '@CallLLM.tool_name', // Dynamically call the tool the LLM requested.
                    'if' => '@CallLLM.type == "tool_use"',
                    'input_map' => '@CallLLM.tool_input', // Pass the input the LLM provided.
                ],
                [
                    'id' => 'AppendToolResult',
                    'tool' => 'UpdateState',
                    'if' => '@CallLLM.type == "tool_use"',
                    'params' => ['conversation_history' => '@State.conversation_history + {"role": "user", "content": {"type": "tool_result", "content": "@ExecuteToolIfRequested.output"}}'],
                ],
                [
                    'id' => 'CheckForContinuation',
                    'tool' => 'UpdateState',
                    // If a tool was used, loop again. If not, stop.
                    'params' => ['loop_continue' => '@CallLLM.type == "tool_use"'],
                ],
            ]
        ]
    ];

    echo "Starting AI Code Agent (Orchestrator Mode)...\n";
    
    // A single function call executes the entire, complex, stateful,
    // looping, and tool-using logic.
    $result = PipelineOrchestrator::run([], $pipelineDefinition);

    if (!$result->isSuccess()) {
        echo "Pipeline FAILED: " . $result->getErrorMessage() . "\n";
    }

} catch (QuicproException | \Exception $e) {
    die("A fatal orchestrator error occurred: " . $e->getMessage() . "\n");
}
~~~

# Use Case: The Code Knowledge Engine Agent

This document outlines the architecture of one of the most powerful components in the `quicpro_async` ecosystem: the **Knowledge Graph Agent**. This is not a simple file storage service; it is a specialized microservice that parses, understands, and versions source code, transforming a flat directory of files into a rich, queryable, semantic graph.

## 1. The Vision: Code as a Living Data Structure

Traditional version control systems like Git are excellent at tracking changes to lines of text, but they have no understanding of the code's actual structure or meaning. It is impossible to ask Git questions like:
* "Show me all methods in this project that are not covered by a unit test."
* "Which parts of the code were discussed most frequently in relation to performance bugs last month?"
* "Find all deprecated methods that are still being called by our frontend services."

To answer these questions, we need to treat our codebase not as a collection of text files, but as a deeply interconnected graph of classes, methods, properties, and relationships. This "living" graph, when annotated with conversational and operational data, becomes the ultimate source of truth and the foundation for truly intelligent developer tools.

## 2. The Architecture: The AST-to-Graph Pipeline

The `KnowledgeGraphAgent` is an MCP server that acts as the central point for ingesting and managing this code graph. Its workflow is triggered, for example, by a `git commit` hook.

1.  **Code Ingestion:** A commit hook sends the content of changed files to the agent.
2.  **AST Parsing:** The agent uses a powerful parser library (like `nikic/php-parser`) to transform the raw PHP source code into an **Abstract Syntax Tree (AST)**. This AST is a structured representation of the code, breaking it down into its logical components (namespaces, classes, methods, function calls, etc.).
3.  **Graph Transformation:** The agent traverses the AST and converts it into a graph structure suitable for a graph database like Neo4j. Each element (class, method) becomes a node, and the relationships (e.g., `EXTENDS`, `IMPLEMENTS`, `CALLS`, `HAS_METHOD`) become edges.
4.  **Versioning & Annotation:** Each set of changes is wrapped in a `Commit` node, creating a versioned history within the graph. Crucially, other services can now call this agent to **annotate** the graph, linking conversation snippets, bug reports from a ticketing system, or performance metrics to specific nodes (methods or classes) in the code graph.

### Mermaid Diagram: The Knowledge Graph Ingestion Flow

~~~mermaid
graph TD
    subgraph "Development & CI/CD"
        A(fa:fa-code Developer commits code);
        B(fa:fa-git-alt Git Commit Hook);
        A --> B;
    end

    subgraph "quicpro_async Backend"
        C["fa:fa-cogs KnowledgeGraph Agent<br/>(PHP + MCP Server)<br/> - "];
        D("fa:fa-sitemap PHP Parser Library<br/>(nikic/php-parser)<br/> - ");
        E[("fa:fa-database Neo4j Database<br/>(Stores the AST Graph with decaying annotations)<br/> - ")];
    end

    subgraph "External Systems"
       F("fa:fa-comments Team Chat / Discussions<br/> - ");
       G(fa:fa-ticket-alt Ticketing System);
    end

    B -- "1. MCP Call: commitFile(content)<br/> - " --> C;
    C -- "2. Uses" --> D;
    D -- "3. Returns AST" --> C;
    C -- "4. Writes Cypher Queries" --> E;
    
    F -- "Annotate Code<br/>(MCP Call)<br/> - " --> C;
    G -- "Link Issue<br/>(MCP Call)<br/> - " --> C;

    class A,B,F,G external;
    class C,D,E internal;
~~~

## 3. Implementation: The `KnowledgeGraphAgent`

The following script is a complete example of the agent. It requires `nikic/php-parser` and a Neo4j driver, which would be installed via Composer. The core logic resides in the `commitFile` method, which recursively traverses the AST.

~~~php
<?php
// FILENAME: agents/knowledge_graph_agent.php
declare(strict_types=1);

if (!extension_loaded('quicpro_async')) { die("Quicpro extension not loaded.\n"); }

// This agent requires external PHP libraries, which would be managed by Composer.
require __DIR__ . '/vendor/autoload.php';

use Quicpro\Config;
use Quicpro\MCP\Server as McpServer;
use Quicpro\IIBIN;
use Quicpro\Exception\QuicproException;
use PhpParser\Node;
use PhpParser\NodeTraverser;
use PhpParser\NodeVisitorAbstract;
use PhpParser\ParserFactory;
use Laudis\Neo4j\ClientBuilder;
use Laudis\Neo4j\Contracts\ClientInterface;

/**
 * The service that contains the logic for parsing code and updating the graph.
 */
class KnowledgeGraphService
{
    private \PhpParser\Parser $parser;
    private ClientInterface $neo4jClient;

    public function __construct()
    {
        $this->parser = (new ParserFactory)->create(ParserFactory::PREFER_PHP7);
        $this->neo4jClient = ClientBuilder::create()
            ->withDriver('bolt', 'bolt://user:password@localhost:7687')
            ->build();
    }

    /**
     * Receives file content, parses it into an AST, and stores it in Neo4j.
     */
    public function commitFile(string $payload): string
    {
        $request = IIBIN::decode('FileCommitRequest', $payload);
        $filePath = $request['path'];
        $fileContent = $request['content'];

        try {
            $ast = $this->parser->parse($fileContent);
            $this->storeAstInGraph($filePath, $ast);
        } catch (\PhpParser\Error $e) {
            return IIBIN::encode('CommitResponse', ['status' => 'PARSE_ERROR', 'error' => $e->getMessage()]);
        } catch (\Throwable $e) {
            return IIBIN::encode('CommitResponse', ['status' => 'GRAPH_ERROR', 'error' => $e->getMessage()]);
        }

        return IIBIN::encode('CommitResponse', ['status' => 'SUCCESS']);
    }

    private function storeAstInGraph(string $filePath, array $ast): void
    {
        // This is a simplified traversal. A real implementation would be more detailed.
        $traverser = new NodeTraverser();
        $visitor = new class($this->neo4jClient, $filePath) extends NodeVisitorAbstract {
            private ClientInterface $client;
            private string $filePath;
            public function __construct(ClientInterface $client, string $filePath) {
                $this->client = $client;
                $this->filePath = $filePath;
                // Create a file node
                $this->client->run('MERGE (f:File {path: $path})', ['path' => $this->filePath]);
            }
            public function enterNode(Node $node) {
                if ($node instanceof Node\Stmt\Class_) {
                    $this->client->run(
                        'MATCH (f:File {path: $filePath}) ' .
                        'MERGE (c:Class {name: $name}) ' .
                        'MERGE (f)-[:CONTAINS]->(c)',
                        ['filePath' => $this->filePath, 'name' => (string)$node->name]
                    );
                } elseif ($node instanceof Node\Stmt\ClassMethod) {
                    $className = $node->getAttribute('parent')->name;
                    $this->client->run(
                        'MATCH (c:Class {name: $className}) ' .
                        'MERGE (m:Method {name: $methodName}) ' .
                        'MERGE (c)-[:HAS_METHOD]->(m)',
                        ['className' => (string)$className, 'methodName' => (string)$node->name]
                    );
                }
            }
        };
        $traverser->addVisitor($visitor);
        $traverser->traverse($ast);
    }
}

// --- Main Server Execution ---
try {
    IIBIN::defineSchema('FileCommitRequest', ['path' => ['type' => 'string', 'tag' => 1], 'content' => ['type' => 'string', 'tag' => 2]]);
    IIBIN::defineSchema('CommitResponse', ['status' => ['type' => 'string', 'tag' => 1], 'error' => ['type' => 'string', 'tag' => 2, 'optional' => true]]);

    $serverConfig = Config::new([/* mTLS config here */]);
    $server = new McpServer('127.0.0.1', 9903, $serverConfig);
    
    $kgService = new KnowledgeGraphService();
    $server->registerService('KnowledgeGraph', $kgService);
    
    echo "Knowledge Graph Agent listening on udp://127.0.0.1:9903...\n";
    $server->run();

} catch (QuicproException | \Exception $e) {
    die("A fatal agent error occurred: " . $e->getMessage() . "\n");
}
~~~

