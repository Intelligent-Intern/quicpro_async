# Use Case: A Decoupled, Web-Based AI Code Agent

This document details the implementation of a powerful, code-editing AI agent. Inspired by the concept of an LLM with tools, this example goes beyond a simple script and builds a robust, decoupled microservices architecture using `quicpro_async`. This approach is significantly more scalable, secure, and maintainable than a monolithic implementation.

## 1. The Architectural Vision: The Decoupled Agent

The core principle is a clean separation of concerns. The user-facing component should not be responsible for sensitive operations like filesystem access or handling secret API keys. Our architecture decouples these roles into specialized, independent agents that communicate via the high-performance Model Context Protocol (MCP).

1.  **The Main Agent (WebSocket Server):** This is the "brain" of the operation and the primary user-facing component. It is a stateful `quicpro_async` WebSocket server that manages conversations with multiple concurrent users connected via a web browser. Its only jobs are to maintain the conversation loop and to orchestrate calls to other backend agents to get work done. It has no direct access to the filesystem or any secret API keys.
2.  **The Filesystem Agent (MCP Server):** This is a secure, sandboxed microservice that exposes filesystem operations (`read`, `list`, `edit`) as a clean MCP API. It is the "hands" of the agent. It can be run with restricted user permissions and sandboxed to a specific working directory for security.
3.  **The Anthropic API Agent (MCP Server):** This is a secure "proxy" or "bridge" service. It is the only component in the system that holds the `ANTHROPIC_API_KEY`. It takes requests from the Main Agent, adds the secret key, and securely forwards them to the external Anthropic API.

### Mermaid Diagram: The Decoupled Agent Architecture

This diagram shows how the user interacts with the Main Agent via WebSocket, which in turn orchestrates the backend tool agents via MCP.

~~~mermaid
graph LR
    %% --- Style Definitions ---
    classDef user fill:#ef6c00,color:white
    classDef main_agent fill:#0097a7,stroke:#006064,stroke-width:4px,color:white
    classDef tool_agent fill:#512da8,color:white,stroke-width:2px
    classDef external fill:#424242,color:white
    classDef filesystem fill:#4caf50,color:white

    subgraph "User Interface"
        A(fa:fa-user User in Browser);
    end

    subgraph "quicpro_async Backend"
        B["fa:fa-brain Main Agent<br/>(WebSocket Server & Orchestrator)<br/>-"];
        C["fa:fa-tools Filesystem Agent<br/>(MCP Server)<br/>-"];
        D["fa:fa-key Anthropic API Agent<br/>(MCP Server)<br/>-"];
    end
    
    subgraph "External & System Resources"
        E(fa:fa-robot Anthropic API);
        F(fa:fa-folder-open Local Filesystem);
    end

    %% --- Data Flows ---
    A -- "1. Persistent WebSocket Connection" <--> B;
    B -- "2. MCP Call:<br/>readFile('main.php')" --> C;
    B -- "3. MCP Call:<br/>runInference(conversation)" --> D;
    C -- "Performs OS calls" --> F;
    D -- "Secure HTTPS Call<br/>(adds API key)" --> E;

    class A user;
    class B main_agent;
    class C,D tool_agent;
    class E external;
    class F filesystem;
~~~

## 2. Implementation: The Agent Fleet

The following are the complete, "expert-level" PHP scripts for the two primary backend components.

### Component 1: The Filesystem Agent (`filesystem_agent.php`)

This is a standalone MCP server that exposes safe filesystem operations. It uses the high-level, event-driven server API for clean and simple logic.

~~~php
<?php
// FILENAME: agents/filesystem_agent.php
declare(strict_types=1);

if (!extension_loaded('quicpro_async')) { die("Quicpro extension not loaded.\n"); }

use Quicpro\Config;
use Quicpro\MCP\Server as McpServer;
use Quicpro\IIBIN;
use Quicpro\Exception\QuicproException;

class FilesystemService
{
    private string $workDir;

    public function __construct(string $workDir)
    {
        $this->workDir = realpath($workDir) ?: throw new \InvalidArgumentException("Invalid working directory: {$workDir}");
        echo "Filesystem Agent sandboxed to: {$this->workDir}\n";
    }

    private function securePath(string $relativePath): string|false
    {
        $fullPath = realpath($this->workDir . '/' . $relativePath);
        if ($fullPath === false || strpos($fullPath, $this->workDir) !== 0) {
            return false;
        }
        return $fullPath;
    }
    
    public function list_files(string $payload): string
    {
        $input = IIBIN::decode('PathRequest', $payload);
        $path = $this->securePath($input['path'] ?? '.');
        if ($path === false) { return IIBIN::encode('ErrorResponse', ['error' => 'Access denied.']); }
        
        $files = scandir($path);
        return IIBIN::encode('FileListResponse', ['files' => array_values(array_diff($files, ['.', '..']))]);
    }

    public function read_file(string $payload): string
    {
        $input = IIBIN::decode('PathRequest', $payload);
        $path = $this->securePath($input['path']);
        if ($path === false || !is_file($path)) { return IIBIN::encode('ErrorResponse', ['error' => 'File not found or access denied.']); }

        return IIBIN::encode('FileContentResponse', ['content' => file_get_contents($path)]);
    }

    public function edit_file(string $payload): string
    {
        $input = IIBIN::decode('EditFileRequest', $payload);
        $path = $this->securePath($input['path']);
        if ($path === false) { return IIBIN::encode('ErrorResponse', ['error' => 'Access denied.']); }

        $content = is_file($path) ? file_get_contents($path) : '';
        if ($input['old_str'] === '' && $content !== '') {
             return IIBIN::encode('ErrorResponse', ['error' => 'Cannot create a file that already exists with edit_file.']);
        }

        $newContent = str_replace($input['old_str'], $input['new_str'], $content);
        file_put_contents($path, $newContent);
        return IIBIN::encode('SuccessResponse', ['status' => 'OK']);
    }
}

try {
    // Schemas should be defined in a shared bootstrap file.
    IIBIN::defineSchema('PathRequest', ['path' => ['type' => 'string', 'optional' => true, 'tag' => 1]]);
    IIBIN::defineSchema('FileListResponse', ['files' => ['type' => 'repeated_string', 'tag' => 1]]);
    IIBIN::defineSchema('FileContentResponse', ['content' => ['type' => 'string', 'tag' => 1]]);
    IIBIN::defineSchema('EditFileRequest', ['path' => ['type' => 'string', 'tag' => 1], 'old_str' => ['type' => 'string', 'tag' => 2], 'new_str' => ['type' => 'string', 'tag' => 3]]);
    IIBIN::defineSchema('SuccessResponse', ['status' => ['type' => 'string', 'tag' => 1]]);
    IIBIN::defineSchema('ErrorResponse', ['error' => ['type' => 'string', 'tag' => 1]]);

    $serverConfig = Config::new([/* mTLS config for security would go here */]);
    $server = new McpServer('127.0.0.1', 9901, $serverConfig);
    
    $fsService = new FilesystemService(getcwd());
    $server->registerService('Filesystem', $fsService);
    
    echo "🚀 Filesystem Agent listening on udp://127.0.0.1:9901...\n";
    $server->run();

} catch (QuicproException | \Exception $e) {
    die("A fatal error occurred in Filesystem Agent: " . $e->getMessage() . "\n");
}
~~~

### Component 2: The Main Agent (WebSocket Server & Orchestrator)

This is the user-facing server. It uses the event-driven `WebSocket\Server` API to handle clients and MCP clients to orchestrate the backend agents.

~~~php
<?php
// FILENAME: main_agent_server.php
declare(strict_types=1);

if (!extension_loaded('quicpro_async')) { die("Quicpro extension not loaded.\n"); }

use Quicpro\Config;
use Quicpro\WebSocket\Server as WebSocketServer;
use Quicpro\WebSocket\Connection;
use Quicpro\MCP;
use Quicpro\IIBIN;
use Quicpro\Exception\QuicproException;

class AgentOrchestrator
{
    private MCP $anthropicAgent;
    private MCP $filesystemAgent;
    private array $toolsForApi;

    public function __construct()
    {
        $this->anthropicAgent = new MCP('127.0.0.1', 9902, Config::new());
        $this->filesystemAgent = new MCP('127.0.0.1', 9901, Config::new());

        // Define the tools for the LLM, mapping them to our MCP services.
        $this->toolsForApi = [
            ['name' => 'list_files', 'description' => 'List files and directories.', 'input_schema' => IIBIN::getSchemaDefinition('PathRequest')],
            ['name' => 'read_file', 'description' => 'Read the contents of a file.', 'input_schema' => IIBIN::getSchemaDefinition('PathRequest')],
            ['name' => 'edit_file', 'description' => 'Create or edit a file by replacing text.', 'input_schema' => IIBIN::getSchemaDefinition('EditFileRequest')],
        ];
    }

    // Handles the entire conversation loop for a single WebSocket client.
    public function handleConversation(Connection $clientConnection, string $initialMessage): void
    {
        $conversationHistory = [['role' => 'user', 'content' => $initialMessage]];

        while (true) {
            // Step 1: Call the LLM via the secure Anthropic Agent
            $llmRequest = IIBIN::encode('AnthropicRequest', ['messages' => $conversationHistory, 'tools' => $this->toolsForApi]);
            $llmResponsePayload = $this->anthropicAgent->request('AnthropicBridge', 'proxyMessage', $llmRequest);
            $llmResponse = IIBIN::decode('AnthropicResponse', $llmResponsePayload);
            
            $conversationHistory[] = ['role' => 'assistant', 'content' => $llmResponse['content']];

            // Step 2: Process the LLM's response
            $toolUseBlock = null;
            $textResponse = '';
            foreach ($llmResponse['content'] as $contentBlock) {
                if ($contentBlock['type'] === 'text') {
                    $textResponse .= $contentBlock['text'];
                } elseif ($contentBlock['type'] === 'tool_use') {
                    $toolUseBlock = $contentBlock;
                    // For simplicity, we handle the first tool use request. A more advanced
                    // agent could handle multiple parallel tool calls.
                    break;
                }
            }
            
            // Send any text response back to the user immediately.
            if (!empty($textResponse)) {
                $clientConnection->send(json_encode(['type' => 'text', 'content' => $textResponse]));
            }
            
            // Step 3: If a tool is requested, execute it via the Filesystem Agent
            if ($toolUseBlock) {
                $clientConnection->send(json_encode(['type' => 'tool_call', 'name' => $toolUseBlock['name'], 'input' => $toolUseBlock['input']]));
                
                $requestPayload = IIBIN::encode(ucfirst($toolUseBlock['name']).'Request', $toolUseBlock['input']);
                $responsePayload = $this->filesystemAgent->request('Filesystem', $toolUseBlock['name'], $requestPayload);
                $toolResult = IIBIN::decode(ucfirst($toolUseBlock['name']).'Response', $responsePayload);
                
                // Add the tool result to the conversation history and loop to ask the LLM what to do next.
                $conversationHistory[] = ['role' => 'user', 'content' => [['type' => 'tool_result', 'tool_use_id' => $toolUseBlock['id'], 'content' => json_encode($toolResult)]]];
            } else {
                // If there's no tool to use, the conversation turn is over. Break the loop.
                break;
            }
        }
    }
}

try {
    // Schemas used by this agent. Should be in a shared bootstrap file.
    IIBIN::defineSchema('AnthropicRequest', ['messages' => ['type' => 'repeated_map', 'tag' => 1], 'tools' => ['type' => 'repeated_map', 'tag' => 2]]);
    IIBIN::defineSchema('AnthropicResponse', ['content' => ['type' => 'repeated_map', 'tag' => 1]]);

    $serverConfig = Config::new(['cert_file' => './certs/server.pem', 'key_file' => './certs/server.pem']);
    $wsServer = new WebSocketServer('0.0.0.0', 4433, $serverConfig);
    
    $orchestrator = new AgentOrchestrator();

    $wsServer->on('message', function (Connection $connection, string $message) use ($orchestrator) {
        // For each new user message, we start a new conversation handling Fiber.
        // This allows the server to handle many users concurrently.
        (new Fiber([$orchestrator, 'handleConversation']))->start($connection, $message);
    });

    echo "🚀 Main Code Agent (WebSocket Hub) listening on udp://0.0.0.0:4433...\n";
    $wsServer->run();

} catch (QuicproException | \Exception $e) {
    die("A fatal error occurred in Main Agent Server: " . $e->getMessage() . "\n");
}
~~~

~~~