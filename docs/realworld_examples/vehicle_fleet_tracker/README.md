## 1. Introduction: Building a Real-Time Fleet Management Platform

This series of documents provides a comprehensive, expert-level blueprint for architecting and implementing a complete, end-to-end fleet management platform. The goal is to move beyond simple asset tracking and create a mission-critical, data-driven system for a modern logistics operation. The business drivers are clear: reducing fuel costs through route optimization, enabling predictive maintenance by analyzing engine diagnostics in real-time, improving driver safety by monitoring behavior, and providing customers with accurate, live ETAs. To achieve this, we must build a system that can handle a "firehose" of data from thousands of vehicles, process it with low latency, and store it for both real-time and long-term analysis.

The architecture is founded on the principles of a decoupled, scalable microservices design. A monolithic approach, where a single application handles all tasks, would be brittle and inefficient. Instead, we will construct a system of specialized, independent services. This strategy provides critical advantages:
* **Independent Scalability:** The Ingestion Gateway, which terminates network connections, might be CPU-bound by TLS processing. The Persistence Agent, in contrast, is I/O-bound by the database. A decoupled architecture allows us to scale each component independently - for example, adding more Ingestion nodes to handle a growing fleet without altering the persistence layer.
* **Fault Isolation & Resilience:** An issue with a downstream service, such as a database slowdown, must not impact the system's ability to ingest data from the fleet. By separating these concerns, the Ingestion Gateway can continue to accept data and queue it for processing, ensuring zero data loss even if a persistence node is temporarily unavailable.
* **Technological Specialization:** Each service does one thing and does it well. The Ingestion Gateway is a "dumb but fast" network endpoint. The Persistence Agent is an expert in efficient database batching. The Live Map Hub is an expert in managing the state of thousands of concurrent WebSocket connections. This focus simplifies development, testing, and maintenance.

A key architectural decision is the exclusive use of the **Model Context Protocol (MCP)** for all inter-service communication, even for tasks not directly related to AI. In this ecosystem, we treat every microservice as an **"Agent"** - a specialized entity that provides a discrete tool or capability. This approach creates a unified communication bus. Whether a service is writing to a database, performing OCR, or querying an LLM, it uses the same high-performance binary protocol, simplifying the system's network topology. More importantly, this standardization lays the groundwork for a future of autonomous orchestration. By exposing all system capabilities as a consistent set of tools, we enable future AI models to dynamically discover and combine these agents to solve novel problems, far beyond the initially programmed workflows.

The entire backend infrastructure for this complex, distributed system is built using **PHP**, supercharged by the **`quicpro_async`** framework. This choice demonstrates a new paradigm for PHP, moving it from a traditional web scripting language into the realm of high-performance systems programming. By choosing PHP, an organization can leverage its existing talent pool and a vast ecosystem, while `quicpro_async` provides the missing C-native performance engine. The framework's key roles in this architecture are: providing the multi-process `Cluster` supervisor to run all services robustly; offering the high-performance QUIC transport layer to handle unreliable mobile networks; enabling the stateful WebSocket server for the frontend; and powering the efficient MCP inter-service communication bus. For storage, we use a PostgreSQL database enhanced with two powerful extensions: **TimescaleDB** for hyper-efficient time-series data management ("when") and **PostGIS** for advanced geospatial querying ("where"). This combination allows for sophisticated analytical queries that would be impossibly slow on a traditional database.

In the following chapters, we will deconstruct this system piece by piece. We will begin with the foundational database schema, then build each microservice - the vehicle client, the ingestion gateway, the persistence agent, and the real-time visualization hub. Each section will provide detailed architectural diagrams, expert configurations, and the complete, runnable source code, demonstrating how to build an industry-standard, scalable system with PHP.

## 2. Overall System Architecture

The following diagram provides a high-level overview of the entire Fleet Management Platform. It illustrates the relationships between the four main components: the **Fleet Clients**, the central **Backend** services, the **Persistence Layer**, and the **Frontend Consumers**. This architecture is designed for scalability and resilience by decoupling responsibilities into specialized services that communicate via the Model Context Protocol (MCP). Each component is a standalone application that can be developed, deployed, and scaled independently.

In the subsequent sections, we will break down each of these components, highlighting the relevant part of this diagram and providing the complete source code and configuration for it.

~~~mermaid
graph TD
%% --------------------------------------------------------------------------
%% --- STYLE DEFINITIONS ---
%% --------------------------------------------------------------------------
    classDef iot fill:#00796b,color:white
    classDef router fill:#006064,color:white,stroke-width:4px
    classDef gateway fill:#0097a7,stroke:#006064,stroke-width:2px,color:white
    classDef hub_cluster fill:#d84315,color:white,stroke-width:2px
    classDef frontend fill:#ef6c00,color:white
    classDef persistence fill:#4527a0,color:white,stroke-width:2px
    classDef db fill:#512da8,color:white,stroke-width:4px
    classDef ai_cluster fill:#c2185b,color:white,stroke-width:4px
    classDef knowledge fill:#f3e5f5,stroke:#880e4f,stroke-width:2px,color:#311b92
    classDef external fill:#424242,color:white

%% --------------------------------------------------------------------------
%% --- TIER 1: CLIENTS & DNS ---
%% --------------------------------------------------------------------------
    subgraph "TIER 1: Clients & Intelligent DNS"
        A(fa:fa-user-friends End Customers);
        B(fa:fa-truck Vehicle Fleet);
        DNS["fa:fa-dns Smart DNS Server<br/>(PHP + Quicpro)<br/>-"];
        HealthAgent["fa:fa-heartbeat Cluster Health Agent<br/>(MCP Service)<br/>-"];
        A --> DNS;
        B --> DNS;
        DNS -- "MCP Call:<br/>GetHealthyRouters()<br/>-" --> HealthAgent;
    end

%% --------------------------------------------------------------------------
%% --- TIER 2: STATELESS QUIC ROUTER CLUSTER ---
%% --------------------------------------------------------------------------
    subgraph "TIER 2: Stateless QUIC Router Cluster"
        direction LR
        R1["fa:fa-random QUIC Router 1"];
        R2["fa:fa-random QUIC Router 2"];
        Rn["fa:fa-random ... Router N"];
    end
    HealthAgent -- "Monitors Routers & Hubs" --> R1 & R2 & Rn;

%% --------------------------------------------------------------------------
%% --- TIER 3: STATEFUL APPLICATION SERVICES ---
%% --------------------------------------------------------------------------
subgraph "TIER 3: Stateful Application Services (PHP + Quicpro Clusters)"
direction LR

subgraph "Ingestion & Persistence"
direction TB
IG["fa:fa-sitemap Ingestion Gateway"];
PA["fa:fa-database-plus Persistence Agents"];
DB[(fa:fa-server-rack DB Cluster)];
IG -- MCP --> PA;
PA -- SQL --> DB;
end

subgraph "AI & Analytics Platform"
direction TB
Q([fa:fa-tasks Job Queue]);
AIC["fa:fa-server AI Worker Cluster"];
KG[("fa:fa-brain Knowledge Graph<br/>& Vector DB<br/>-")];
AIC -- "Reads/Writes" --> KG
Q -.-> AIC
end

subgraph "Live Operations Hub"
direction TB
Hub["fa:fa-satellite-dish Live Operations Hub Cluster<br/>(WebSocket + MCP)<br/>-"];
FE("fa:fa-map-marked-alt User Interfaces<br/>(Dispatch, Chat, Driver App)<br/>-");
Hub -- "Real-time Push" --> FE
end

subgraph "External 3rd-Party Integrations"
direction TB
TrafficAPI("fa:fa-traffic-light Real-time Traffic API");
WeatherAPI("fa:fa-cloud-sun-rain Weather API");
TicketSystem("fa:fa-ticket-alt Customer Ticket System<br/>(e.g., Zendesk, Jira)<br/>-");
end
end

%% --------------------------------------------------------------------------
%% --- DATA FLOWS ---
%% --------------------------------------------------------------------------
DNS -- "Returns healthy router IP" --> A & B;
A & B -- "Initial QUIC Connection" --> R1;
R1 -- "Forwards UDP via<br/>Consistent Hashing of<br/>Connection ID<br/>-" --> IG & Hub;

IG -- "Publishes Telemetry Event (MCP)" --> Hub;
IG -- "Sends Data for Storage (MCP)" --> PA;
DB -- "DB Triggers/Events" --> Q;

Hub -- "RAG/ML Queries (MCP)" --> AIC;
AIC -- "New Route/Insight (MCP)" --> Hub;

TrafficAPI -- "Provides Data to" --> AIC;
WeatherAPI -- "Provides Data to" --> AIC;
TicketSystem -- "Events (e.g. 'Cancel Delivery')" --> AIC;


%% --- APPLY STYLES ---
class A,B,FE frontend;
class DNS,HealthAgent dns;
class R1,R2,Rn router;
class IG gateway;
class PA persistence;
class DB db;
class Hub hub_cluster;
class Q queue;
class AIC ai_cluster;
class KG knowledge;
class TrafficAPI,WeatherAPI,TicketSystem external;
~~~


~~~mermaid

graph TD
%% --------------------------------------------------------------------------
%% --- STYLE DEFINITIONS ---
%% --------------------------------------------------------------------------
    classDef iot fill:#00796b,color:white
    classDef gateway fill:#0097a7,stroke:#006064,stroke-width:4px,color:white
    classDef hub fill:#d84315,color:white,stroke-width:4px
    classDef persistence fill:#4527a0,color:white,stroke-width:2px
    classDef db fill:#512da8,color:white,stroke-width:4px
    classDef frontend fill:#ef6c00,color:white
    classDef ai_cluster fill:#c2185b,color:white,stroke-width:4px
    classDef knowledge fill:#f3e5f5,stroke:#880e4f,stroke-width:2px,color:#311b92
    classDef external fill:#424242,color:white

%% --------------------------------------------------------------------------
%% --- TIER 1: EXTERNAL SYSTEMS & CLIENTS ---
%% --------------------------------------------------------------------------
    subgraph "TIER 1: External Systems & Clients"
        A(fa:fa-truck Vehicle Fleet);
        B(fa:fa-user-friends End Customers);

        subgraph "External Data Feeds"
            direction TB
            API_Traffic("fa:fa-traffic-light Real-Time Traffic API");
            API_Weather("fa:fa-cloud-sun-rain Weather API");
            API_ERP("fa:fa-ticket-alt ERP / Ticketing System");
        end
    end

%% --------------------------------------------------------------------------
%% --- TIER 2: INGESTION & REAL-TIME ROUTING (PHP + QUICPRO ASYNC) ---
%% --------------------------------------------------------------------------
subgraph "TIER 2: Ingestion, Persistence & Real-time Hub"
direction TB

Ingestion["fa:fa-sitemap Ingestion Gateway Cluster"];
Hub["fa:fa-satellite-dish Live Operations Hub Cluster"];
Persistence["fa:fa-database-plus Persistence Agent Cluster"];
DB["(fa:fa-server-rack Database Cluster<br/>(Postgres/Timescale/PostGIS)<br/>-)"];

A -- QUIC Telemetry --> Ingestion;
Ingestion -- Raw Events (MCP) --> Hub;
Ingestion -- Data for Storage (MCP) --> Persistence;
Persistence -- Batched SQL Writes --> DB;
end

%% --------------------------------------------------------------------------
%% --- TIER 3: AI & ANALYTICS PLATFORM (PHP + QUICPRO ASYNC) ---
%% --------------------------------------------------------------------------
subgraph "TIER 3: AI & Analytics Platform"
direction TB

subgraph "Core AI Services (MCP Agents)"
RTO["fa:fa-route Route Optimization Agent"];
JAA["fa:fa-clipboard-list Job Assignment Agent"];
PMA["fa:fa-car-battery Predictive Maintenance Agent"];
end

KG[("fa:fa-brain Knowledge Graph<br/>(Neo4j + VectorDB)<br/>-")];
DB -- DB Triggers / Events --> KG;
RTO & JAA & PMA -- Reads/Writes --> KG;
end

%% --------------------------------------------------------------------------
%% --- TIER 4: FRONTEND & DRIVER INTERFACE ---
%% --------------------------------------------------------------------------
subgraph "TIER 4: Frontend & Driver Interface"
UI_Dispatch(fa:fa-map-marked-alt Dispatcher UI);
UI_Driver(fa:fa-mobile-alt Driver App);
B --> UI_Dispatch;
A -- Interacts with --> UI_Driver;
end

%% --------------------------------------------------------------------------
%% --- DATA FLOWS & INTERACTIONS ---
%% --------------------------------------------------------------------------
API_Traffic --> RTO;
API_Weather --> RTO;
API_ERP -- "e.g., Order Cancellation" --> JAA;
JAA -- "Updated Job List" --> RTO;

RTO -- "Optimized Route (MCP)" --> Hub;
Hub -- "Push New Route (WebSocket)" --> UI_Driver;
Hub -- "Push Live Positions (WebSocket)" --> UI_Dispatch;

%% --- APPLY STYLES ---
class A,UI_Driver iot;
class B,UI_Dispatch frontend;
class Ingestion,Hub,Persistence gateway;
class DB db;
class RTO,JAA,PMA ai_agent;
class KG knowledge;
class API_Traffic,API_Weather,API_ERP external;
        
~~~~



~~~mermaid
graph TD
%% --------------------------------------------------------------------------
%% --- STYLE DEFINITIONS ---
%% --------------------------------------------------------------------------
    classDef iot fill:#00796b,color:white
    classDef dns fill:#03a9f4,stroke:#01579b,stroke-width:4px,color:white
    classDef router fill:#006064,color:white,stroke-width:2px
    classDef gateway fill:#0097a7,stroke:#006064,stroke-width:4px,color:white
    classDef hub fill:#d84315,color:white,stroke-width:2px
    classDef frontend fill:#ef6c00,color:white
    classDef persistence fill:#4527a0,color:white,stroke-width:2px
    classDef db fill:#512da8,color:white,stroke-width:4px
    classDef queue fill:#7e57c2,color:white
    classDef ai_cluster fill:#c2185b,color:white,stroke-width:4px
    classDef ai_agent fill:#f8bbd0,stroke:#c2185b,stroke-width:2px,color:#880e4f
    classDef knowledge fill:#f3e5f5,stroke:#880e4f,stroke-width:2px,color:#311b92

%% --------------------------------------------------------------------------
%% --- TIER 1: CLIENTS & INTELLIGENT DNS ---
%% --------------------------------------------------------------------------
    subgraph "TIER 1: Clients & Service Discovery"
        A(fa:fa-user-friends End Customers);
        B(fa:fa-truck Vehicle Fleet);
        DNS["fa:fa-dns Smart DNS Server Cluster<br/>(PHP + Quicpro)<br/>-"];
        HealthAgent["fa:fa-heartbeat Cluster Health Agent<br/>(MCP Service)<br/>-"];
        A --> DNS;
        B --> DNS;
        DNS -- "MCP Call:<br/>GetHealthyRouters()" --> HealthAgent;
    end

%% --------------------------------------------------------------------------
%% --- TIER 2: STATELESS QUIC ROUTER CLUSTER ---
%% --------------------------------------------------------------------------
    subgraph "TIER 2: Stateless QUIC Router Cluster (No TLS Termination)"
        direction LR
        R1["fa:fa-random QUIC Router 1"];
        R2["fa:fa-random QUIC Router 2"];
        Rn["fa:fa-random ... Router N"];
    end
    HealthAgent -- Monitors --> R1 & R2 & Rn

%% --------------------------------------------------------------------------
%% --- TIER 3: STATEFUL APPLICATION SERVICES ---
%% --------------------------------------------------------------------------
    subgraph "TIER 3: Stateful Application Services (PHP + Quicpro Clusters)"
        direction LR
        subgraph "Ingestion & Persistence"
            direction TB
            IG["fa:fa-sitemap Ingestion Gateway"];
            PA["fa:fa-database-plus Persistence Agents"];
            DB[(fa:fa-server-rack DB Cluster)];
            IG -- MCP --> PA;
            PA -- SQL --> DB;
        end
        subgraph "AI & Enrichment"
            direction TB
            Q([fa:fa-tasks Job Queue]);
            AIC["fa:fa-server AI Worker Cluster"];
            KG[(fa:fa-brain Knowledge Graph<br/>& Vector DB<br/>-)];
            AIC -- Reads/Writes --> KG
            Q -.-> AIC
        end
        subgraph "Real-time Hub"
            direction TB
            Hub["fa:fa-satellite-dish Live Map Hub Cluster"];
            FE(fa:fa-map-marked-alt Frontend UIs);
            Hub -- WebSocket Push --> FE
        end
    end

%% --------------------------------------------------------------------------
%% --- DATA FLOWS ---
%% --------------------------------------------------------------------------
    DNS -- "Returns healthy router IP" --> A & B;
    A -- "WebSocket Connection" --> R1;
    B -- "Telemetry Stream" --> R1;
    R1 -- "Forwards UDP via<br/>Consistent Hashing of<br/>Connection ID" --> IG & Hub;

    IG -- "Publishes Job" --> Q;
    IG -- "Publishes Event" --> Hub;

%% --- APPLY STYLES ---
    class A,B,FE frontend;
    class DNS,HealthAgent dns;
    class R1,R2,Rn router;
    class IG gateway;
    class PA persistence;
    class DB db;
    class Hub hub_cluster;
    class Q queue;
    class AIC ai_cluster;
    class KG knowledge;
~~~

~~~mermaid
graph TD
%% --------------------------------------------------------------------------
%% --- STYLE DEFINITIONS ---
%% --------------------------------------------------------------------------
    classDef iot fill:#00796b,color:white
    classDef router fill:#006064,color:white,stroke-width:4px
    classDef gateway fill:#0097a7,stroke:#006064,stroke-width:2px,color:white
    classDef hub_cluster fill:#d84315,color:white,stroke-width:2px
    classDef frontend fill:#ef6c00,color:white
    classDef persistence fill:#4527a0,color:white,stroke-width:2px
    classDef db fill:#512da8,color:white,stroke-width:4px

%% --------------------------------------------------------------------------
%% --- TIER 1: CLIENTS & DNS ---
%% --------------------------------------------------------------------------
    subgraph "TIER 1: Clients & Service Discovery"
        A(fa:fa-user-friends End Customers);
        B(fa:fa-truck Vehicle Fleet);
        DNS("fa:fa-dns DNS Round-Robin<br/>(live.my-company.com)<br/>-");
        A --> DNS;
        B --> DNS;
    end

%% --------------------------------------------------------------------------
%% --- TIER 2: STATELESS QUIC ROUTER CLUSTER ---
%% --------------------------------------------------------------------------
    subgraph "TIER 2: Stateless QUIC Router Cluster (No TLS Termination)"
        C["fa:fa-random QUIC Router 1<br/>(PHP + Quicpro)<br/>-"];
        D["fa:fa-random QUIC Router 2<br/>(PHP + Quicpro)<br/>-"];
        E["fa:fa-random ...<br/>QUIC Router N<br/>-"];
    end

%% --------------------------------------------------------------------------
%% --- TIER 3: STATEFUL BACKEND SERVICES ---
%% --------------------------------------------------------------------------
    subgraph "TIER 3: Stateful Backend Services (PHP + Quicpro Clusters)"
        direction LR

        subgraph "Ingestion & Persistence"
            direction TB
            F["fa:fa-sitemap Ingestion Gateway"];
            G["fa:fa-database-plus Persistence Agents"];
            H[(fa:fa-server-rack DB Cluster)];
            F -- MCP --> G;
            G -- SQL --> H;
        end

        subgraph "Live Real-Time Hub"
            direction TB
            I["fa:fa-satellite-dish Live Map Hub Cluster<br/>(100+ Servers)<br/>-"];
            J(fa:fa-map-marked-alt Frontend UIs);
            I -- WebSocket Push --> J;
        end
    end

%% --------------------------------------------------------------------------
%% --- DATA FLOWS ---
%% --------------------------------------------------------------------------

    DNS -- "Resolves to one Router IP" --> C;
    DNS -- "..." --> D;
    DNS -- "..." --> E;

    A -- "1. WebSocket Connection<br/>(Initial QUIC Packet)<br/>-" --> C;
    B -- "1. Telemetry Stream<br/>(Initial QUIC Packet)<br/>-" --> C;

    C -- "2. Forwards UDP based on<br/>Consistent Hashing of Connection ID<br/>-" --> F & I;

    F -- "3a. Publishes event via MCP" --> I;
    I -- "3b. Publishes to other Hubs via MCP" --> I;

%% --- APPLY STYLES ---
    class A,B,J frontend;
    class DNS external;
    class C,D,E router;
    class F gateway;
    class G persistence;
    class H db;
    class I hub_cluster;
~~~

# but round robin is no longer needed - we c an give feedback to DNS via MCP for true DNS loadbalancing

# Use Case: Building a Service-Oriented DNS Server with MCP

This document details the architecture for one of the most advanced and powerful use cases for the `quicpro_async` framework: building a high-performance, intelligent DNS server that is itself a fully integrated microservice within the application ecosystem.

## 1. The Problem: Static DNS in a Dynamic World

Traditional DNS is a relatively static system. It maps a hostname to a list of IP addresses based on a pre-configured zone file. While techniques like DNS Round-Robin offer simple load distribution, they are fundamentally "dumb". They have no awareness of the real-time health, load, or capacity of the servers they point to. If a server crashes or becomes overloaded, a standard DNS server will continue to send clients to it, leading to connection failures and downtime.

Modern cloud providers solve this with proprietary, black-box "smart DNS" services that offer health checks and geo-routing, but these are external dependencies that lock you into a specific vendor's ecosystem.

## 2. The `quicpro_async` Solution: DNS as a Native Microservice

We will not build a new DNS protocol. Instead, we will build a DNS **server** that fully complies with the DNS standard on UDP port 53, but whose logic is powered by our own microservice architecture. This is DNS 2.0.

**The Architecture:**

1.  **The Smart DNS Server:** This is a `quicpro_async` application running as a cluster. Its only job is to listen for standard DNS queries. It is lean, fast, and contains no business logic itself.

2.  **The Cluster Health Agent:** This is a separate MCP service that acts as the "source of truth" for the entire infrastructure. It continuously monitors the health and load of all other services (e.g., the QUIC Routers, the Live Map Hubs).

3.  **The MCP-driven Lookup:** When the Smart DNS Server receives a query for `live.my-company.com`, it does not look in a local file. Instead, it makes a high-speed, non-blocking **MCP call** to the Cluster Health Agent, asking, "What are the 5 healthiest and least-loaded IP addresses for the 'live-map-hubs' service right now?". The Health Agent returns a list of the best available IPs.

4.  **The Dynamic Response:** The Smart DNS server takes this real-time list of IPs, constructs a valid DNS response packet, and sends it back to the client.

**The Advantages:**

* **Real-Time, Intelligent Load Balancing:** This system provides true, real-time, health-check-aware load balancing. It will never return the IP of a dead or overloaded server.
* **Complete Autonomy:** The entire system is self-contained and self-managing, eliminating the need for external, proprietary DNS services or complex service discovery tools like Consul.
* **Infinite Flexibility:** The logic for choosing the "best" IP can be as simple or as complex as needed. It could be based on server load, geographic proximity (if the client's subnet is passed in the MCP call), or even business logic like routing premium customers to premium infrastructure.
* **A Groundbreaking Demonstration for PHP:** A performant, asynchronous DNS server capable of handling tens of thousands of queries per second - written in PHP - would fundamentally change the perception of what the language is capable of.

### Mermaid Diagram: The Smart DNS Architecture

This diagram shows how the DNS server is no longer a static configuration file but an active participant in the microservice ecosystem.

~~~mermaid
graph TD
    %% --- Style Definitions ---
    classDef client fill:#ef6c00,color:white
    classDef dns fill:#03a9f4,stroke:#01579b,stroke-width:4px,color:white
    classDef agent fill:#f3e5f5,stroke:#512da8,stroke-width:2px,color:#311b92
    classDef backend fill:#0097a7,color:white

    subgraph "External World"
        A(fa:fa-user-friends Clients);
    end

    subgraph "Our Infrastructure (Powered by Quicpro Async)"
        
        subgraph "TIER 1: Smart DNS Tier"
            B["fa:fa-dns Smart DNS Server Cluster<br/>(Listens on UDP Port 53)<br/>-"];
        end

        subgraph "TIER 2: Backend Services"
            direction LR
            C["fa:fa-heartbeat Cluster Health Agent<br/>(MCP Server)<br/>-"];
            D["fa:fa-server Application Server Cluster<br/>(e.g., QUIC Routers, Hubs)<br/>-"];
        end
        
        C -- "Monitors Health & Load" --> D;
    end

    %% --- Data Flow ---
    A -- 1. Standard DNS Query for 'live.my-company.com' --> B;
    B -- "2. MCP Request:<br/>'GetHealthyNodes(live-map-hubs)'" --> C;
    C -- "3. MCP Response:<br/>[IP1, IP2, IP3]" --> B;
    B -- "4. Dynamic DNS Response" --> A;

    %% --- Apply Styles ---
    class A client;
    class B dns;
    class C agent;
    class D backend;
~~~



~~~mermaid
graph TD
    %% --------------------------------------------------------------------------
    %% --- STYLE DEFINITIONS ---
    %% --------------------------------------------------------------------------
    classDef iot fill:#00796b,color:white
    classDef gateway fill:#0097a7,stroke:#006064,stroke-width:4px,color:white
    classDef persistence fill:#4527a0,color:white,stroke-width:2px
    classDef db fill:#512da8,color:white,stroke-width:4px
    classDef hub_cluster fill:#d84315,color:white,stroke-width:2px
    classDef frontend fill:#ef6c00,color:white
    
    %% --------------------------------------------------------------------------
    %% --- ARCHITECTURE OVERVIEW ---
    %% --------------------------------------------------------------------------
    
    subgraph "CLIENTS"
        A("fa:fa-truck Vehicle Fleet<br/>(100,000+)<br/>-");
        B("fa:fa-user-friends End Customers<br/>(10,000,000+)<br/>-");
    end

    subgraph "BACKEND SERVICES (Powered by Quicpro Async Clusters)"
        direction LR

        subgraph "Ingestion & Persistence Pipeline"
            direction TB
            C["fa:fa-sitemap Telemetry Ingestion Gateway<br/>(Receives 20k events/sec)<br/>-"];
            D["fa:fa-database-plus Persistence Agents<br/>(Multi-Master Writers)<br/>-"];
            E[("fa:fa-server-rack Persistence DB Cluster<br/>(Postgres/Timescale/PostGIS)<br/>-")];
            C -- "1. MCP Round-Robin" --> D;
            D -- "2. Batched Writes" --> E;
        end
        
        subgraph "Real-Time Fan-Out Hub (Horizontally Scaled)"
            direction TB
            F["fa:fa-satellite-dish Live Map Hub Cluster<br/>(100+ PHP Servers)<br/>-"];
            G("fa:fa-arrows-alt-h L4 Load Balancer<br/>(for WebSockets)<br/>-");
            G --> F;
        end
    end

    %% --------------------------------------------------------------------------
    %% --- DATA FLOWS ---
    %% --------------------------------------------------------------------------
    
    A -- "Telemetry (QUIC / IIBIN)" --> C;
    B -- "WebSocket Connect" --> G;
    
    C -- "3. MCP BROADCAST to ALL 100+ Hub nodes<br/>(IIBIN Payload)<br/>-" --> F;
    
    F -- "4. Each Hub checks local subscribers<br/>& pushes GeoJSON if a match is found<br/>-" --> G;
    
    %% --- APPLY STYLES ---
    class A,B iot;
    class C gateway;
    class D persistence;
    class E db;
    class F hub_cluster;
    class G frontend;
~~~

~~~mermaid
graph TD
%% --------------------------------------------------------------------------
%% --- STYLE DEFINITIONS ---
%% --------------------------------------------------------------------------
    classDef iot fill:#00796b,color:white
    classDef gateway fill:#0097a7,stroke:#006064,stroke-width:4px,color:white
    classDef persistence fill:#4527a0,color:white,stroke-width:2px
    classDef db fill:#512da8,color:white,stroke-width:4px
    classDef hub_cluster fill:#d84315,color:white,stroke-width:2px
    classDef frontend fill:#ef6c00,color:white

%% --------------------------------------------------------------------------
%% --- ARCHITECTURE OVERVIEW ---
%% --------------------------------------------------------------------------

    subgraph "CLIENTS"
        direction TB
        A("fa:fa-truck Vehicle Fleet");
        B("fa:fa-user-friends 10M End Customers");
    end

    subgraph "BACKEND SERVICES (Powered by Quicpro Async Clusters)"
        direction LR

        subgraph "Ingestion & Persistence"
            direction TB
            C["fa:fa-sitemap Telemetry Ingestion Gateway"];
            D["fa:fa-database-plus Persistence Agents<br/>(Multi-Master Writers)<br/>-"];
            E[("fa:fa-server-rack Persistence DB Cluster<br/>(Postgres/Timescale/PostGIS)<br/>-")];
            C -- MCP Round-Robin --> D;
            D -- Batched Writes --> E;
        end

        subgraph "Real-Time Fan-Out Hub"
            direction TB
            F["fa:fa-satellite-dish Live Map Hub Cluster<br/>(100+ PHP Servers)<br/>-"];
            G("fa:fa-arrows-alt-h L4 Load Balancer<br/>(for WebSockets)<br/>-");
            G --> F;
        end
    end

%% --------------------------------------------------------------------------
%% --- DATA FLOWS ---
%% --------------------------------------------------------------------------

    A -- "1. Telemetry (QUIC)" --> C;
    B -- "2. WebSocket Connect" --> G;

    F -- "3. On Subscribe<br/>(e.g. to TRUCK-123)<br/>-" --o D;
    D -- "4. Writes:<br/>'routing:TRUCK-123' -> 'hub_server_56'" --> F;

    C -- "5. On Telemetry<br/>GET 'routing:TRUCK-123'<br/>-" --> D;
    D -- "6. Returns 'hub_server_56'" --> C;

    C -- "7. Direct MCP Call" --> F;

    F -- "8. WebSocket Push" --> G;

%% --- APPLY STYLES ---
    class A,B iot;
    class C gateway;
    class D persistence;
    class E db;
    class F hub_cluster;
    class G frontend;
~~~


~~~mermaid
graph LR
    %% --------------------------------------------------------------------------
    %% --- STYLE DEFINITIONS ---
    %% --------------------------------------------------------------------------
    classDef iot fill:#00796b,color:white
    classDef gateway fill:#0097a7,stroke:#006064,stroke-width:4px,color:white
    classDef persistence fill:#4527a0,color:white,stroke-width:2px
    classDef db fill:#512da8,color:white,stroke-width:4px
    classDef queue fill:#7e57c2,color:white
    classDef ai_cluster fill:#c2185b,color:white,stroke-width:4px
    classDef ai_agent fill:#f8bbd0,stroke:#c2185b,stroke-width:2px,color:#880e4f
    classDef knowledge fill:#f3e5f5,stroke:#880e4f,stroke-width:2px,color:#311b92
    classDef hub fill:#d84315,color:white,stroke-width:4px
    classDef frontend fill:#ef6c00,color:white
    classDef external fill:#424242,color:white

    %% --------------------------------------------------------------------------
    %% --- REGION 1: FLEET (DATA SOURCE) ---
    %% --------------------------------------------------------------------------
    subgraph "REGION 1: FLEET"
        A("fa:fa-truck Vehicle Client<br/>(PHP + Quicpro)<br/>-");
    end

    %% --------------------------------------------------------------------------
    %% --- REGION 2: INGESTION & PERSISTENCE BACKEND ---
    %% --------------------------------------------------------------------------
    subgraph "REGION 2: INGESTION & PERSISTENCE"
        direction TB
        B["fa:fa-sitemap Telemetry Ingestion Gateway<br/>(PHP Cluster)<br/>-"];
        
        subgraph "Persistence Cluster"
            direction LR
            PA["fa:fa-database-plus Persistence Agents<br/>(Round-Robin Pool)<br/>-"];
            DB["(fa:fa-server-rack DB Cluster<br/>(Postgres/Timescale/PostGIS)<br/>-)"];
            PA -- Batched Writes --> DB;
        end
        
        Q(["fa:fa-tasks Job Queue<br/>(For AI Enrichment)<br/>-"]);
        
        B -- Raw Telemetry (MCP) --> PA;
        PA -- Write Success Event --> Q;
    end

    %% --------------------------------------------------------------------------
    %% --- REGION 3: AI & ANALYTICS PLATFORM ---
    %% --------------------------------------------------------------------------
    subgraph "REGION 3: AI & ANALYTICS"
        direction TB
        
        AI_Cluster["fa:fa-server AI Worker Cluster<br/>(Consumes from Job Queue)<br/>-"];
        
        subgraph "AI Agents (MCP Microservices)"
            direction LR
            AI_RTO["fa:fa-route Route<br/>Optimizer<br/>-"];
            AI_PMA["fa:fa-car-battery Predictive<br/>Maintenance<br/>-"];
            AI_GNN["fa:fa-sitemap GNN<br/>Trainer<br/>-"];
        end
        
        KNOWLEDGE_BASE["(fa:fa-brain Knowledge Graph<br/>& Vector DB<br/>-)"];
        
        AI_Cluster -- Orchestrates --> AI_RTO;
        AI_Cluster -- Orchestrates --> AI_PMA;
        AI_Cluster -- Orchestrates --> AI_GNN;
        AI_RTO & AI_PMA & AI_GNN -- Read/Write --> KNOWLEDGE_BASE
    end

    %% --------------------------------------------------------------------------
    %% --- REGION 4: REAL-TIME HUB & FRONTEND ---
    %% --------------------------------------------------------------------------
    subgraph "REGION 4: REAL-TIME HUB & FRONTEND"
        direction TB
        
        subgraph "Live Map & Command Hub (Stateful PHP Cluster)"
            Hub_MCP["fa:fa-network-wired MCP Server<br/>(Receives Live Events)<br/>-"];
            Hub_WS["fa:fa-globe-americas WebSocket Server<br/>(Serves UI Clients)<br/>-"];
            Hub_MCP -- Pub/Sub Fan-Out --> Hub_WS;
        end

        subgraph "User & Driver Interfaces"
            UI_Dispatch(fa:fa-map-marked-alt Dispatcher UI);
            UI_Driver(fa:fa-mobile-alt Driver App);
        end
        
        Hub_WS -- GeoJSON Push --> UI_Dispatch;
        Hub_WS -- New Route Push --> UI_Driver;
    end
    
    %% --------------------------------------------------------------------------
    %% --- GLOBAL DATA FLOWS ---
    %% --------------------------------------------------------------------------
    A -- QUIC Telemetry Stream --> B;
    B -- Publish Event (MCP) --> Hub_MCP;
    Q -.-> AI_Cluster;
    AI_RTO -- Optimized Route (MCP) --> Hub_MCP;
    
    %% --- Apply Styles ---
    class A iot;
    class B,Hub_MCP,Hub_WS gateway;
    class PA,DB persistence;
    class Q queue;
    class AI_Cluster,AI_RTO,AI_PMA,AI_GNN ai_cluster;
    class KNOWLEDGE_BASE knowledge;
    class UI_Dispatch,UI_Driver frontend;
~~~

~~~mermaid
graph LR
    %% --------------------------------------------------------------------------
    %% --- STYLE DEFINITIONS ---
    %% --------------------------------------------------------------------------
    classDef device fill:#00796b,color:white,stroke:#fff,stroke-width:2px
    classDef cluster fill:#0097a7,stroke:#006064,stroke-width:2px,color:white
    classDef process fill:#b2ebf2,stroke:#0097a7,stroke-width:2px,color:#004d40
    classDef fiber fill:#ffffff,stroke:#0097a7,stroke-width:2px,stroke-dasharray: 3 3,color:black
    classDef persistence fill:#4527a0,color:white,stroke-width:2px
    classDef db fill:#512da8,color:white,stroke-width:4px
    classDef queue fill:#7e57c2,color:white
    classDef ai_cluster fill:#c2185b,color:white,stroke-width:4px
    classDef ai_agent fill:#f8bbd0,stroke:#c2185b,stroke-width:2px,color:#880e4f
    classDef knowledge fill:#f3e5f5,stroke:#880e4f,stroke-width:2px,color:#311b92
    classDef hub fill:#d84315,color:white,stroke-width:4px
    classDef frontend fill:#ef6c00,color:white
    classDef external fill:#424242,color:white


    %% --------------------------------------------------------------------------
    %% --- 1. THE FLEET (IOT CLIENTS) ---
    %% --------------------------------------------------------------------------
    subgraph "1. On-Board Vehicle Clients"
        CLIENT["fa:fa-truck In-Vehicle App<br/>(tracker.php)<br/>-"]
        subgraph "Client Internals"
            direction TB
            CLIENT_SESS[("Quicpro\Session<br/>(Resilient QUIC Connection)<br/>-")];
            CLIENT_GPS[("fa:fa-satellite-dish GPS Device<br/>(/dev/ttyS0)<br/>-")];
            CLIENT_CAN[("fa:fa-car-battery CAN Bus Reader")];
        end
        CLIENT_GPS -- NMEA Stream --> CLIENT;
        CLIENT_CAN -- Engine Data --> CLIENT;
        CLIENT -- Creates & Uses --> CLIENT_SESS;
    end
    
    %% --------------------------------------------------------------------------
    %% --- 2. INGESTION & PERSISTENCE BACKEND ---
    %% --------------------------------------------------------------------------
    subgraph "2. Ingestion & Persistence Backend"
        direction TB
        
        subgraph "Ingestion Gateway (PHP Cluster)"
            INGEST_CLUSTER["fa:fa-server Quicpro\Cluster<br/>(Manages Ingestion Workers)<br/>-"];
            subgraph "Worker Process (ingestion_worker.php)"
                INGEST_WORKER["fa:fa-cogs Worker Process"];
                subgraph "Inside Worker Fiber"
                    INGEST_FIBER("fa:fa-microchip Fiber<br/>(Handles one vehicle session)<br/>-");
                end
                INGEST_WORKER -- "Spawns for each vehicle" --> INGEST_FIBER;
            end
            INGEST_CLUSTER -- "fork()" --> INGEST_WORKER;
        end

        subgraph "Persistence Cluster (PHP Cluster)"
            PERSIST_CLUSTER["fa:fa-server Quicpro\Cluster<br/>(Manages DB Writers)<br/>-"];
            subgraph "Worker Process (db_writer_agent.php)"
                 PERSIST_WORKER["fa:fa-cogs Worker Process"];
                 subgraph "Inside DB Worker"
                     PERSIST_AGENT["fa:fa-database-plus Persistence Agent<br/>(MCP Server with Batching)<br/>-"];
                     DB_CONN[("fa:fa-plug PostgreSQL<br/>Connection Pool<br/>-")];
                     PERSIST_AGENT -- "Batched INSERTs" --> DB_CONN;
                 end
                 PERSIST_WORKER -- "Runs" --> PERSIST_AGENT
            end
            PERSIST_CLUSTER -- "fork()" --> PERSIST_WORKER
        end
        
        DB_CLUSTER["(fa:fa-server-rack Database Cluster<br/>(Postgres + TimescaleDB + PostGIS)<br/>-)"];
        JOB_QUEUE("[fa:fa-tasks Job Queue<br/>(For async enrichment)<br/>-]");
        
        INGEST_FIBER -- "1. Non-blocking MCP Call<br/>(Round-Robin)" --> PERSIST_AGENT;
        INGEST_FIBER -- "2. Publish Job" --> JOB_QUEUE;
        DB_CONN --> DB_CLUSTER;
    end

    %% --------------------------------------------------------------------------
    %% --- 3. AI & ANALYTICS PLATFORM ---
    %% --------------------------------------------------------------------------
    subgraph "3. AI & Analytics Platform (Decoupled)"
        direction TB
        
        AI_WORKER_CLUSTER["fa:fa-server Quicpro\Cluster<br/>(Manages Enrichment Workers)<br/>-"];
        subgraph "Enrichment Worker Process"
            AI_WORKER["fa:fa-cogs Worker Process<br/>(Consumes from Job Queue)<br/>-"];
            AI_ORCHESTRATOR[("fa:fa-project-diagram PipelineOrchestrator<br/>(Drives AI toolchain)<br/>-")]
            AI_WORKER -- "Executes" --> AI_ORCHESTRATOR
        end
        AI_WORKER_CLUSTER -- "fork()" --> AI_WORKER
        
        subgraph "AI Agents (Independent MCP Services)"
            direction LR
            AI_OCR[fa:fa-file-image OCR Agent];
            AI_LLM[fa:fa-brain Local LLM Agent];
            AI_GDBW[fa:fa-pen-nib GraphDB Writer];
            AI_RTO[fa:fa-route Route Optimizer];
            AI_PMA[fa:fa-car-battery Predictive Maintenance];
            AI_GNN[fa:fa-sitemap GNN Agent];
        end
        
        KNOWLEDGE_BASE["(fa:fa-brain Knowledge Graph & VectorDB<br/>(Neo4j, etc.)<br/>-)"];
        EXTERNAL_APIS("fa:fa-traffic-light External APIs<br/>(Traffic, Weather)<br/>-");

        AI_ORCHESTRATOR -- MCP Calls --> AI_OCR & AI_LLM & AI_GDBW;
        AI_GDBW -- Writes graph --> KNOWLEDGE_BASE;
        AI_RTO & AI_PMA & AI_GNN -- Read/Write/Train on --> KNOWLEDGE_BASE;
        EXTERNAL_APIS --> AI_RTO;
    end

    %% --------------------------------------------------------------------------
    %% --- 4. OPERATIONS & VISUALIZATION HUB ---
    %% --------------------------------------------------------------------------
    subgraph "4. Operations & Visualization Hub"
        direction TB
        
        HUB_CLUSTER["fa:fa-server Quicpro\Cluster<br/>(Manages Hub Workers)<br/>-"];
        subgraph "Hub Worker Process (live_map_hub.php)"
            HUB_WORKER["fa:fa-cogs Worker Process"];
            subgraph "Inside Hub Worker (Concurrent Servers)"
                HUB_WS("fa:fa-globe-americas WebSocket Server<br/>(Handles Frontend Clients)<br/>-");
                HUB_MCP("fa:fa-network-wired MCP Server<br/>(Receives real-time events)<br/>-");
            end
            HUB_WORKER -- "Runs in parallel Fibers" --> HUB_WS & HUB_MCP;
        end
        HUB_CLUSTER -- "fork()" --> HUB_WORKER;
        
        subgraph "User & Driver Interfaces"
            UI_DISPATCH("fa:fa-map-marked-alt Dispatcher Dashboard<br/>(Vue.js)<br/>-");
            UI_CHAT("fa:fa-comments RAG Chat UI");
        end
    end

    %% --------------------------------------------------------------------------
    %% --- GLOBAL DATA FLOWS ---
    %% --------------------------------------------------------------------------
    CLIENT_SESS -- Unidirectional QUIC Stream<br/>(IIBIN Payload) --> INGEST_WORKER;
    JOB_QUEUE -.-> AI_WORKER;
    INGEST_FIBER -- "Publish Event (MCP)" --> HUB_MCP;
    HUB_WS -- WebSocket Push<br/>(GeoJSON) --> UI_DISPATCH;
    AI_RTO -- Optimized Route (MCP) --> HUB_MCP;
    HUB_WS -- New Route via WebSocket --> CLIENT;
    UI_CHAT -- RAG Query --> HUB_MCP;

    %% --- Apply Styles ---
    class CLIENT,CLIENT_SESS,CLIENT_GPS,CLIENT_CAN device;
    class INGEST_CLUSTER,PERSIST_CLUSTER,HUB_CLUSTER,AI_WORKER_CLUSTER cluster;
    class INGEST_WORKER,PERSIST_WORKER,HUB_WORKER,AI_WORKER process;
    class INGEST_FIBER,HUB_WS,HUB_MCP fiber;
    class PERSIST_AGENT,DB_CONN persistence;
    class DB_CLUSTER db;
    class JOB_QUEUE queue;
    class AI_ORCHESTRATOR,AI_OCR,AI_LLM,AI_GDBW,AI_RTO,AI_PMA,AI_GNN ai_agent;
    class KNOWLEDGE_BASE knowledge;
    class UI_DISPATCH,UI_CHAT frontend;
    class EXTERNAL_APIS external;
~~~


