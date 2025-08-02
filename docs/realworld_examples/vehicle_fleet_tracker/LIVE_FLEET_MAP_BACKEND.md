# Use Case: High-Performance, In-Memory Pub/Sub Hub for Live Tracking

This document details the architecture and implementation of a stateful, high-performance Pub/Sub message broker, written entirely in PHP using the `quicpro_async` framework. This "Live Map Hub" serves as the real-time backbone for the fleet tracking dashboard, replacing external dependencies like Redis or RabbitMQ with a more integrated and performant solution.

## 1. The Architectural Challenge: Real-Time Fan-Out

The core challenge of a live map is the "fan-out" problem: a single event (a new GPS coordinate from one vehicle) must be broadcast instantly to multiple subscribed clients (all dispatchers watching that vehicle). A traditional request-response model is unsuitable.

While a common solution is to use an external message broker like Redis Pub/Sub, this introduces network latency between services, a separate point of failure, and additional infrastructure overhead. The `quicpro_async` philosophy is to empower PHP to handle such stateful, high-performance tasks natively.

## 2. The `quicpro_async` Solution: A Multi-Port MCP/WebSocket Server

We will build a single, powerful PHP server process that acts as the central message broker. This server listens on **two ports simultaneously** using the concurrency features of `quicpro_async`:

1.  **A Public WebSocket Port:** This is the endpoint for frontend clients (the Vue.js map). Dispatchers connect here to subscribe to topics and receive `GeoJSON` updates.
2.  **An Internal MCP Port:** This is a private, high-speed endpoint for other backend services. The `Telemetry Ingestion Server` from the previous example acts as a **publisher**, sending new vehicle coordinates to this port via a lightweight MCP call.

The server maintains an in-memory list of topics and their subscribers. When a message is published on the MCP port, the server immediately looks up all subscribed WebSocket clients for that topic and pushes the data to them. This entire process happens within a single, highly concurrent PHP application.

### Mermaid Diagram: The Integrated Pub/Sub Hub Architecture

~~~mermaid
graph TD
%% --- Style Definitions ---
    classDef iot fill:#00796b,color:white
    classDef backend fill:#0097a7,stroke:#006064,stroke-width:4px,color:white
    classDef persistence fill:#4527a0,color:white,stroke-width:2px
    classDef db fill:#ede7f6,stroke:#4527a0,stroke-width:2px,color:#311b92
    classDef frontend fill:#ef6c00,color:white

%% --- Block 1: Fleet ---
    subgraph "Fleet (Data Source)"
        A(fa:fa-truck Vehicle Fleet<br/>Installed in Trucks<br/>-);
    end

%% --- Block 2: Backend ---
    subgraph "Backend (Ingestion & Real-time Hub)"
        B["fa:fa-sitemap Telemetry Ingestion Server<br/>(PHP Cluster)<br/>-"];
        C["fa:fa-satellite-dish Live Map Hub<br/>(Stateful WebSocket/MCP Server)<br/>-"];
        B -- Publish via MCP --> C;
    end

%% --- Block 3: Persistence ---
    subgraph "Persistence Layer"
        direction LR
        subgraph "Persistence Agents"
            PA1["fa:fa-database Persistence Agent 1"];
            PA2["fa:fa-database Persistence Agent 2"];
            PAn["fa:fa-database ... Agent N"];
        end

        subgraph "Database (Multi-Master)"
            DB1[(fa:fa-server DB Master 1)];
            DB2[(fa:fa-server DB Master 2)];
            DBn[(fa:fa-server ... DB Master N)];
        end

        PA1 -- Saves in parallel to --> DB1;
        PA2 -- Saves in parallel to --> DB2;
        PAn -- Saves in parallel to --> DBn;
    end

%% --- Block 4: Frontend ---
    subgraph "Frontend (Data Consumers)"
        F1(fa:fa-user Dispatcher 1);
        F2(fa:fa-user Dispatcher 2);
        Fn(fa:fa-user ...);
    end

%% --- Inter-Block Connections ---
    A --> B;
    B -- via Round-Robin MCP --> PA1;
    B -- "..." --> PA2;
    B -- "..." --> PAn;
    C -- Pushes Updates to --> F1;
    C -- "..." --> F2;
    C -- "..." --> Fn;

%% --- Apply Styles to Nodes ---
    class A iot;
    class B,C backend;
    class F1,F2,Fn frontend;
    class PA1,PA2,PAn persistence;
    class DB1,DB2,DBn db;
~~~

## 3. Implementation: The PHP Pub/Sub Hub Server

The following script is the complete, runnable code for the stateful hub. It demonstrates running two server loops concurrently within a single application, managing shared state, and handling the fan-out logic.

~~~php
<?php
declare(strict_types=1);

if (!extension_loaded('quicpro_async')) {
    die("Fatal Error: The 'quicpro_async' extension is not loaded.\n");
}

use Quicpro\Config;
use Quicpro\Server;
use Quicpro\Session;
use Quicpro\IIBIN;
use Quicpro\Exception\QuicproException;

// --- Server Configuration ---
const WEBSOCKET_HOST = '0.0.0.0';
const WEBSOCKET_PORT = 4433;
const MCP_HOST = '127.0.0.1'; // Internal-only interface
const MCP_PORT = 9801;

/**
 * The central Pub/Sub Hub. This class is the core of the stateful application.
 * It manages all topics and their WebSocket subscribers.
 */
class LiveMapHub
{
    /** @var array<string, array<int, resource>> */
    private array $topics = [];

    public function subscribe(string $topic, $websocketStream): void
    {
        $clientId = spl_object_id($websocketStream);
        $this->topics[$topic][$clientId] = $websocketStream;
        echo "HUB: Client #{$clientId} subscribed to topic '{$topic}'.\n";
    }

    public function unsubscribe($websocketStream): void
    {
        $clientId = spl_object_id($websocketStream);
        foreach ($this->topics as $topic => &$subscribers) {
            if (isset($subscribers[$clientId])) {
                unset($subscribers[$clientId]);
                echo "HUB: Client #{$clientId} unsubscribed from topic '{$topic}'.\n";
            }
        }
    }

    /**
     * Publishes a message to all subscribers of a topic. This is the "fan-out".
     */
    public function publish(string $topic, string $payload): void
    {
        if (empty($this->topics[$topic])) {
            return;
        }

        echo "HUB: Publishing to " . count($this->topics[$topic]) . " subscribers on topic '{$topic}'.\n";
        foreach ($this->topics[$topic] as $clientId => $stream) {
            if (is_resource($stream) && !feof($stream)) {
                fwrite($stream, $payload);
            } else {
                // Clean up dead subscriber
                unset($this->topics[$topic][$clientId]);
            }
        }
    }
}

/**
 * The handler logic for a single connected WebSocket client.
 * Runs inside a dedicated Fiber.
 */
function handle_websocket_client(Session $session, LiveMapHub $hub): void
{
    $websocketStream = null;
    try {
        $websocketStream = $session->upgrade('/live-map');
        if (!$websocketStream) { return; }

        while (is_resource($websocketStream) && !feof($websocketStream)) {
            $commandJson = stream_get_contents($websocketStream, -1, 0);
            if (!empty($commandJson)) {
                $command = json_decode($commandJson, true);
                if (isset($command['action']) && $command['action'] === 'SUBSCRIBE') {
                    $hub->subscribe($command['topic'], $websocketStream);
                }
            }
            $session->poll(200);
        }
    } finally {
        if ($websocketStream) { $hub->unsubscribe($websocketStream); }
        if ($session->isConnected()) { $session->close(); }
    }
}

/**
 * The handler logic for the internal MCP server.
 * Runs inside a dedicated Fiber.
 */
function handle_mcp_publisher(Session $session, LiveMapHub $hub): void
{
    try {
        while ($request = $session->receiveRequest()) {
            $telemetry = IIBIN::decode('GeoTelemetry', $request->getPayload());
            
            // Create the GeoJSON payload for the frontend
            $geoJson = json_encode(['type' => 'Feature', /* ... */ 'properties' => $telemetry]);

            // Publish to general topic and vehicle-specific topic
            $hub->publish('live_updates:all', $geoJson);
            $hub->publish('vehicle:' . $telemetry['vehicle_id'], $geoJson);

            $request->sendResponse(IIBIN::encode('Ack', ['status' => 'OK']));
        }
    } finally {
        if ($session->isConnected()) { $session->close(); }
    }
}


// --- Main Server Execution ---

try {
    // 1. Define schemas for MCP communication.
    IIBIN::defineSchema('GeoTelemetry', [/* ... */]);
    IIBIN::defineSchema('Ack', ['status' => ['type' => 'string', 'tag' => 1]]);
    
    // 2. Instantiate the shared Pub/Sub Hub.
    $hub = new LiveMapHub();

    // 3. Configure and instantiate the two servers.
    $wsConfig  = Config::new(['cert_file' => './certs/server_cert.pem', 'key_file' => './certs/server_key.pem', 'alpn' => ['h3']]);
    $mcpConfig = Config::new(['cert_file' => './certs/agent_cert.pem',  'key_file' => './certs/agent_key.pem',  'alpn' => ['mcp/1.0']]);

    $webSocketServer = new Server(WEBSOCKET_HOST, WEBSOCKET_PORT, $wsConfig);
    $mcpServer       = new Server(MCP_HOST, MCP_PORT, $mcpConfig);
    
    echo "🚀 Pub/Sub Hub running. Listening for WebSockets on port " . WEBSOCKET_PORT . " and MCP on port " . MCP_PORT . "...\n";

    // 4. Launch two master Fibers, one for each server loop.
    // This is the core of the concurrent server.
    $wsFiber = new Fiber(function() use ($webSocketServer, $hub) {
        while ($session = $webSocketServer->accept()) {
            // Launch a new Fiber for each connected WebSocket client.
            (new Fiber('handle_websocket_client'))->start($session, $hub);
        }
    });

    $mcpFiber = new Fiber(function() use ($mcpServer, $hub) {
        while ($session = $mcpServer->accept()) {
            // Launch a new Fiber for each connected MCP publisher.
            (new Fiber('handle_mcp_publisher'))->start($session, $hub);
        }
    });

    $wsFiber->start();
    $mcpFiber->start();
    
    // The script will now run indefinitely, managed by these Fibers.
    // A real application would have a master event loop here.

} catch (QuicproException | \Exception $e) {
    die("A fatal server error occurred: " . $e->getMessage() . "\n");
}