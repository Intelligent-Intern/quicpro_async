# Use Case: High-Throughput IoT Telemetry Ingestion & Storage

This document details the architecture and implementation of a massive-scale, real-time telemetry ingestion platform built entirely with PHP and the `quicpro_async` framework. It showcases how to handle a "firehose" of data from thousands of concurrent clients and persist it efficiently into a specialized time-series database.

## 1. The Business Problem & Technical Challenge

A global logistics company requires a platform to ingest and store real-time telemetry from its fleet of thousands of vehicles. Each vehicle sends a data packet every few seconds containing its precise GPS location, speed, and engine diagnostics.

The core challenges are:

* **Massive Concurrency:** The system must handle tens of thousands of persistent QUIC connections simultaneously.
* **High Ingestion Rate:** The platform needs to process a constant, high-velocity stream of incoming data packets without dropping any.
* **Database Performance:** Writing every single incoming packet directly to a database can easily overload it, causing write-contention, high latency, and creating a bottleneck for the entire system.
* **Efficient Storage & Querying:** The data must be stored in a way that allows for both ultra-fast time-series analysis (e.g., "show the route of truck X over the last 48 hours") and complex geospatial queries (e.g., "which vehicles entered this geofence yesterday?").

## 2. The `quicpro_async` Architecture: Decoupled Ingestion & Persistence

To solve these challenges, we design a decoupled microservice architecture where each component is specialized for a single task and can be scaled independently. The communication between these services is handled by MCP, making external message brokers like RabbitMQ or Redis unnecessary.

1.  **The Ingestion Gateway:** This is a cluster of `quicpro_async` PHP workers whose only job is to terminate the QUIC connections from the vehicle fleet. It's the highly-scalable "front door" of the system. Upon receiving a valid telemetry packet, it does not wait for a database write. Instead, it makes a non-blocking, "fire-and-forget" MCP call to the Persistence Agent and immediately goes back to handling network I/O. This ensures that database latency can never slow down the data ingestion from the vehicles.

2.  **The Persistence Agent:** This is a separate, specialized MCP server whose sole responsibility is to interact with the database. It receives telemetry data from the Ingestion Gateway. To prevent overloading the database, this agent can implement smart batching strategies, collecting hundreds of data points in memory before writing them to PostgreSQL in a single, highly efficient transaction.

### The Database Strategy: PostgreSQL + TimescaleDB + PostGIS

This combination creates a best-in-class data backend for this use case:

* **PostgreSQL:** A rock-solid, open-source relational database.
* **TimescaleDB:** A PostgreSQL extension that transforms a standard table into a **hypertable**, a distributed table automatically partitioned by time. This makes time-series queries (e.g., on `timestamp`) orders of magnitude faster.
* **PostGIS:** A PostgreSQL extension that adds support for geographic objects and allows for efficient, indexed geospatial queries.

Yes, **TimescaleDB and PostGIS are fully compatible**. You can create a hypertable on a table that contains a PostGIS `geometry` column. This is the ultimate solution for time-series geospatial data.

### Mermaid Diagram: The Ingestion & Persistence Flow

~~~mermaid
graph TD
    %% --- Style Definitions ---
    classDef iot fill:#e0f7fa,stroke:#00796b,stroke-width:2px,color:#004d40
    classDef agent fill:#f3e5f5,stroke:#512da8,stroke-width:2px,color:#311b92
    classDef db fill:#ede7f6,stroke:#4527a0,stroke-width:4px,color:#311b92

    subgraph "Real-Time Telemetry Platform"
        A(fa:fa-truck Vehicle Fleet) -- QUIC Streams --> B["fa:fa-sitemap Ingestion Gateway<br/>(Quicpro Cluster)<br/> -"];
        B -- Non-blocking MCP Call --> C["fa:fa-database Persistence Agent<br/>(MCP Server with Batching)<br/> -"];
        C -- Batched SQL INSERTs --> D[("fa:fa-server PostgreSQL<br/>TimescaleDB + PostGIS Hypertable<br/> -")];
    end

    class A,B iot;
    class C agent;
    class D db;
~~~

## 3. Implementation

### SQL: Table Structure

This is the SQL required to set up the `vehicle_telemetry` table.

~~~SQL
-- Ensure the required extensions are enabled in your PostgreSQL database.
CREATE EXTENSION IF NOT EXISTS timescaledb;
CREATE EXTENSION IF NOT EXISTS postgis;

-- Create the main table for telemetry data.
CREATE TABLE vehicle_telemetry (
    time        TIMESTAMPTZ       NOT NULL,
    vehicle_id  TEXT              NOT NULL,
    speed_kmh   INTEGER,
    -- The location is stored in a PostGIS geometry column for efficient spatial queries.
    location    GEOMETRY(Point, 4326) NOT NULL
);

-- Create a TimescaleDB hypertable, partitioned by the 'time' column.
-- This will automatically create new partitions (child tables) for new time ranges.
SELECT create_hypertable('vehicle_telemetry', by_range('time'));

-- Create a spatial index on the location column for fast geospatial queries.
CREATE INDEX idx_vehicle_telemetry_location ON vehicle_telemetry USING GIST (location);

-- Create an index on vehicle_id and time for fast lookups of a specific vehicle's path.
CREATE INDEX idx_vehicle_telemetry_vehicle_time ON vehicle_telemetry (vehicle_id, time DESC);
~~~

### PHP: The Persistence Agent (`db_writer_agent.php`)

This is the complete, runnable code for the MCP server that listens for data from the Ingestion Gateway and performs batched writes to the database.

~~~php
<?php
declare(strict_types=1);

if (!extension_loaded('quicpro_async')) {
    die("Fatal Error: The 'quicpro_async' extension is not loaded.\n");
}

use Quicpro\Config;
use Quicpro\Server as McpServer;
use Quicpro\IIBIN;
use Quicpro\Exception\QuicproException;

const AGENT_HOST = '0.0.0.0';
const AGENT_PORT = 9601; // The port this agent listens on.
const DB_CONNECTION_STRING = "host=localhost port=5432 dbname=telemetry user=postgres password=password";

/**
 * The handler class that contains the business logic for this agent.
 */
class PersistenceService
{
    private $dbConnection;
    private array $batch = [];
    private int $batchSize = 200;
    private float $lastWriteTime = 0;
    private int $writeIntervalSec = 2;

    public function __construct()
    {
        // Establish a persistent connection to the database on startup.
        $this->dbConnection = pg_connect(DB_CONNECTION_STRING);
        if (!$this->dbConnection) {
            throw new \RuntimeException("Unable to connect to PostgreSQL database.");
        }
        $this->lastWriteTime = microtime(true);
    }

    /**
     * This is the single RPC method exposed by this agent. It receives a telemetry
     * data point, adds it to an in-memory batch, and triggers a bulk write
     * if the batch is full or a time interval has passed.
     */
    public function recordTelemetry(string $requestPayload): string
    {
        $telemetry = IIBIN::decode('GeoTelemetry', $requestPayload);
        $this->batch[] = $telemetry;

        $currentTime = microtime(true);
        if (count($this->batch) >= $this->batchSize || $currentTime - $this->lastWriteTime >= $this->writeIntervalSec) {
            $this->flushBatch();
            $this->lastWriteTime = $currentTime;
        }

        return IIBIN::encode('Ack', ['status' => 'OK']);
    }

    /**
     * Performs the highly efficient bulk INSERT into the PostgreSQL database.
     */
    private function flushBatch(): void
    {
        if (empty($this->batch)) {
            return;
        }

        pg_query($this->dbConnection, "BEGIN;");
        foreach ($this->batch as $data) {
            pg_query_params(
                $this->dbConnection,
                'INSERT INTO vehicle_telemetry (time, vehicle_id, speed_kmh, location) VALUES ($1, $2, $3, ST_SetSRID(ST_MakePoint($4, $5), 4326));',
                [
                    date(DATE_ATOM, $data['timestamp']),
                    $data['vehicle_id'],
                    $data['speed_kmh'],
                    $data['lon'],
                    $data['lat']
                ]
            );
        }
        pg_query($this->dbConnection, "COMMIT;");

        echo "Flushed " . count($this->batch) . " records to database.\n";
        $this->batch = [];
    }
}

// --- MAIN SERVER EXECUTION ---

try {
    // Define the schemas this agent understands.
    IIBIN::defineSchema('GeoTelemetry', [
        'vehicle_id' => ['type' => 'string', 'tag' => 1],
        'lat'        => ['type' => 'double', 'tag' => 2],
        'lon'        => ['type' => 'double', 'tag' => 3],
        'speed_kmh'  => ['type' => 'uint32', 'tag' => 4],
        'timestamp'  => ['type' => 'uint64', 'tag' => 5],
    ]);
    IIBIN::defineSchema('Ack', ['status' => ['type' => 'string', 'tag' => 1]]);

    // Create a server configuration. For an internal agent, TLS might be optional.
    $serverConfig = Config::new([
        'cert_file' => './certs/agent_cert.pem',
        'key_file'  => './certs/agent_key.pem',
    ]);
    
    // Instantiate and run the MCP server.
    $server = new McpServer(AGENT_HOST, AGENT_PORT, $serverConfig);
    $persistenceService = new PersistenceService();
    $server->registerService('PersistenceService', $persistenceService);
    
    echo "🚀 Persistence Agent listening on udp://" . AGENT_HOST . ":" . AGENT_PORT . "\n";
    $server->run();
    
} catch (QuicproException | \Exception $e) {
    die("A fatal agent error occurred: " . $e->getMessage() . "\n");
}