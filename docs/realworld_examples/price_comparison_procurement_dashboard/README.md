# Use Case: Real-Time B2B Procurement Aggregator

## 1. The Business Case: Supply Chain Optimization in Manufacturing

This document details the implementation of a high-performance, real-time price and availability aggregator, a mission-critical tool for a modern industrial manufacturing company.

Consider a mid-sized German company that builds specialized robotics. Their production line depends on a global supply chain for thousands of electronic components - processors, memory modules, specific sensors, etc. The price and availability of these components are highly volatile, differing significantly between international suppliers (e.g., Digi-Key, Mouser, RS Components, and smaller specialized vendors).

The core business problem is the inefficiency and risk in the procurement process. Currently, when a new production run is planned, a procurement manager must manually check multiple supplier websites or portals to get quotes for the required bill of materials. This process is slow, labor-intensive, and often fails to find the best possible price. More critically, if a preferred supplier is suddenly out of stock of a key component, this manual process can lead to significant production delays, costing the company hundreds of thousands of Euros per day.

The goal is to build an internal **Procurement Dashboard**. When an engineer requests a component by its SKU (e.g., `NVIDIA-JETSON-AGX-ORIN-64GB`), this system must query all registered suppliers in real-time. Within less than a second, it must present the procurement manager with a single, unified view of all available offers, ranked by price, stock level, and estimated delivery time. This allows for instant, data-driven purchasing decisions that optimize cost and guarantee supply chain resilience.

## 2. The Technical Challenge: High-Speed, Concurrent API Orchestration

This business requirement translates into a classic but demanding technical challenge: a **"fan-out/fan-in"** workflow that must execute with extremely low latency.

* **Fan-Out:** A single user request (for one SKU) must trigger multiple, simultaneous API calls to the different, independent supplier systems.
* **Fan-In:** The system must then wait for these parallel requests to return, aggregate the various responses (which may be in different formats), and apply business logic to find the best offer.

A traditional, sequential approach (`call Supplier A -> wait -> call Supplier B -> wait...`) is completely unworkable, as the total response time would be the sum of all individual API latencies, far exceeding the sub-second requirement for an interactive dashboard. The only viable solution is to execute all supplier queries concurrently and to have sophisticated timeout policies to handle slow or unresponsive suppliers without penalizing the user experience.

## 3. The `quicpro_async` Architectural Solution

The `quicpro_async` framework is perfectly suited to solve this problem elegantly and with maximum performance. We will use the C-native **`PipelineOrchestrator`** to define and execute the entire workflow.

The architecture is simple and robust:
1.  **Declarative Pipeline:** The entire fan-out/fan-in logic is defined as a simple, readable PHP array. The core of this is the `parallel` step type, which instructs the C-native orchestrator to execute a list of tasks concurrently using its internal Fiber-based event loop.
2.  **MCP Agents as Adapters:** Each external supplier API is wrapped in a lightweight, specialized **MCP Agent**. This decouples our main application from the complexity and instability of third-party APIs. The orchestrator communicates with these agents using the high-performance, binary `IIBIN` protocol.
3.  **Advanced Completion & Timeout Strategy:** We will configure the `parallel` block with a fine-grained completion policy (e.g., "finish as soon as 3 suppliers have responded, but wait no longer than 600ms in total"). This ensures a fast response while still gathering a sufficient amount of data.
4.  **Aggregation Logic:** A final step in the pipeline calls a simple PHP function to process the collected results and apply the final business logic.

This example will demonstrate several key capabilities of the framework: concurrent I/O, declarative workflows, advanced completion strategies, and the orchestration of decoupled microservices.

## 4. Architectural Diagram: The Fan-Out/Fan-In Pipeline

The following diagram illustrates the decoupled microservices architecture used to solve the price aggregation problem. A central **Orchestrator Client** defines a workflow that makes concurrent requests to multiple, independent **Supplier Agents**. This parallel execution is the key to achieving the sub-second response time required for an interactive user dashboard. After the parallel data gathering is complete, the results are funneled into a final aggregation step to determine the best offer.


~~~mermaid
graph TD
%% Style Definitions
    classDef client fill:#ef6c00,color:white
    classDef discovery fill:#03a9f4,stroke:#01579b,stroke-width:2px,color:white
    classDef router fill:#00BCD4,stroke:#00838f,stroke-width:2px,color:white
    classDef orchestrator fill:#0097a7,stroke:#006064,stroke-width:4px,color:white
    classDef service fill:#512da8,color:white,stroke-width:2px
    classDef agent fill:#b39ddb,stroke:#512da8,stroke-width:1px,color:#311b92
    classDef cluster fill:#006064,color:white
    classDef worker fill:#e0f7fa,stroke:#00796b,stroke-width:1px,color:#004d40
    classDef external fill:#424242,color:white
    classDef db fill:#ede7f6,stroke:#4527a0,stroke-width:2px,color:#311b92
    classDef messaging fill:#7e57c2,color:white

%% TIER 1: User Interface
    subgraph TIER 1: User Interface Layer
        UI["fa:fa-desktop Procurement Dashboard<br/>(Vue.js SPA)<br/>-"];
    end

%% TIER 2: Service Discovery & Ingress Routing
    subgraph TIER 2: Service Discovery and Ingress
        direction LR

        subgraph Smart DNS Cluster
            DNS_Cluster["fa:fa-server Quicpro\Cluster"];
            subgraph "DNS Worker Process"
                DNS_Worker["fa:fa-cogs Worker<br/>(dns_agent.php)<br/>-"];
                DNS_Logic{"fa:fa-question-circle DNS Logic"};
            end
            DNS_Cluster -- "Spawns & Manages" --> DNS_Worker;
            DNS_Worker --> DNS_Logic;
        end

        subgraph Cluster Health Agent
            Health_Cluster["fa:fa-server Quicpro\Cluster<br/>-"];
            subgraph "Health Worker Process"
                Health_Worker["fa:fa-cogs Worker<br/>(health_agent.php)<br/>-"];
                Health_MCP("fa:fa-network-wired MCP Server");
            end
            Health_Cluster -- "Spawns & Manages" --> Health_Worker;
            Health_Worker --> Health_MCP;
        end

        subgraph Stateless QUIC Router Cluster
            Router_Cluster["fa:fa-server Quicpro\Cluster"];
            subgraph "Router Worker Process"
                Router_Worker["fa:fa-cogs Worker<br/>(C-Native Forwarder)<br/>-"];
                Router_Logic("fa:fa-random Consistent Hashing");
            end
            Router_Cluster -- "Spawns & Manages" --> Router_Worker;
            Router_Worker --> Router_Logic
        end
    end

%% TIER 1/2 Flows
    UI -- "DNS Query for 'api.service.internal'" --> DNS_Logic;
    DNS_Logic -- "MCP Call: GetHealthyRouters()" --> Health_MCP;
    Health_MCP -- "MCP Response: [RouterIP1, ...]" --> DNS_Logic;
    DNS_Logic -- "DNS Response" --> UI;
    UI -- "QUIC Connection" --> Router_Worker;

%% Style Applications for Part 1
    class UI client;
    class DNS_Cluster,Health_Cluster,Router_Cluster cluster;
    class DNS_Worker,Health_Worker,Router_Worker worker;
    class DNS_Logic,Health_MCP,Router_Logic agent;

    %% TIER 3: Orchestration & Business Logic Services
    subgraph TIER 3: Orchestration & Business Services
        direction TB

        subgraph "Orchestration Core"
            Orch_Client["fa:fa-play-circle Aggregator Client<br/>(PHP Script)<br/>-"];
            Orchestrator["fa:fa-cogs PipelineOrchestrator<br/>(C-Native Engine)<br/>-"];
            Orch_Client -- "Defines & Runs Pipeline" --> Orchestrator;
        end

        subgraph "Business Microservices (MCP Agent Clusters)"
            direction LR

            subgraph Pricing Service
                Pricing_Cluster["fa:fa-server Quicpro\Cluster"];
                Pricing_Worker["fa:fa-cogs Worker"];
                Pricing_MCP("MCP Server");
                Pricing_Cluster -- "Spawns & Manages" --> Pricing_Worker;
                Pricing_Worker --> Pricing_MCP;
            end
            
            subgraph Inventory Service
                Inv_Cluster["fa:fa-server Quicpro\Cluster"];
                Inv_Worker["fa:fa-cogs Worker"];
                Inv_MCP("MCP Server");
                Inv_Cluster -- "Spawns & Manages" --> Inv_Worker;
                Inv_Worker --> Inv_MCP;
            end

            subgraph FX Rate Service
                FX_Cluster["fa:fa-server Quicpro\Cluster"];
                FX_Worker["fa:fa-cogs Worker"];
                FX_MCP("MCP Server");
                FX_Cluster -- "Spawns & Manages" --> FX_Worker;
                FX_Worker --> FX_MCP;
            end
            
            subgraph Purchase-Order Service
                Order_Cluster["fa:fa-server Quicpro\Cluster"];
                Order_Worker["fa:fa-cogs Worker"];
                Order_MCP("MCP Server");
                Order_Cluster -- "Spawns & Manages" --> Order_Worker;
                Order_Worker --> Order_MCP;
            end
        end
        
        subgraph "Supplier Adapter Cluster (MCP Agents)"
             direction LR
             AdaptA["fa:fa-plug Supplier-A<br/>Adapter<br/>-"];
             AdaptB["fa:fa-plug Supplier-B<br/>Adapter<br/>-"];
             AdaptC["fa:fa-plug Supplier-C<br/>Adapter<br/>-"];
        end

    end

    %% Tier 2 to Tier 3 Flow
    Router_Worker -- "Forwards authenticated request" --> Orch_Client;

    %% Orchestration Flows
    Orchestrator -- "MCP Call: getPrices()" --> Pricing_MCP;
    Orchestrator -- "MCP Call: getInventory()" --> Inv_MCP;
    Orchestrator -- "MCP Call: getFXRates()" --> FX_MCP;
    Orchestrator -- "MCP Call: createOrder()" --> Order_MCP;
    Pricing_MCP -- "MCP Fan-Out to Adapters" --> AdaptA & AdaptB & AdaptC;
    
    %% Style Applications for Part 2
    class Orch_Client orchestrator;
    class Orchestrator orchestrator;
    class Pricing_Cluster,Inv_Cluster,FX_Cluster,Order_Cluster cluster;
    class Pricing_Worker,Inv_Worker,FX_Worker,Order_Worker worker;
    class Pricing_MCP,Inv_MCP,FX_MCP,Order_MCP,AdaptA,AdaptB,AdaptC service;


    %% TIER 4: Data Stores, External APIs & Observability
    subgraph "TIER 4: Data Stores, External Systems & Observability"
        direction TB
        
        subgraph "Data Stores"
            OrdersDB["fa:fa-database Orders DB<br/>(PostgreSQL)<br/>-"];
            HistDB["fa:fa-database Historical Prices<br/>(TimescaleDB)<br/>-"];
        end

        subgraph "External Supplier APIs"
            direction LR
            ExtA("fa:fa-globe Supplier-A API");
            ExtB("fa:fa-globe Supplier-B API");
            ExtC("fa:fa-globe Supplier-C API");
        end

        subgraph "Messaging & Observability (Native & Integrated)"
            direction LR
            EventHub["fa:fa-broadcast-tower MCP Pub/Sub Hub<br/>(Replaces Kafka)<br/>-"];
            Prometheus["fa:fa-chart-bar Built-in<br/>Prometheus Endpoint<br/>-"];
            Grafana("fa:fa-chart-area Grafana");
            Loki("fa:fa-file-alt Loki");
        end
    end

    %% TIER 5: CI/CD, Secrets & Configuration Management
    subgraph TIER 5: CI/CD and Configuration
        direction TB
        CICD("fa:fa-gitlab CI/CD Pipeline");
        SecretsAgent["fa:fa-key Secrets & Config Agent<br/>(MCP Service, replaces Vault/Consul)<br/>-"];
    end

    %% --- Final Data Flows & Connections ---

    %% Adapters to External APIs
    AdaptA -- "HTTPS/REST" --> ExtA;
    AdaptB -- "HTTPS/REST" --> ExtB;
    AdaptC -- "HTTPS/REST" --> ExtC;

    %% Services to Data Stores
    Pricing_MCP -- "Reads historical data" --> HistDB;
    Order_MCP -- "Writes new orders" --> OrdersDB;
    Inv_MCP -- "Reads/Writes stock levels" --> OrdersDB;

    %% Services publishing events to the native Pub/Sub Hub
    Pricing_MCP -- "Publishes 'PriceUpdated' Event" --> EventHub;
    Order_MCP -- "Publishes 'OrderCreated' Event" --> EventHub;
    
    %% Observability Connections
    Prometheus -- "Scrapes" --> Pricing_Cluster & Inv_Cluster & FX_Cluster & Order_Cluster & AdaptA & AdaptB & AdaptC;
    Grafana -- "Queries" --> Prometheus;
    Orch_Client & Pricing_Cluster & Inv_Cluster & FX_Cluster & Order_Cluster & AdaptA & AdaptB & AdaptC -- "Stream Logs to" --> Loki;

    %% CI/CD and Secrets Connections
    CICD -- "Deploys all Agent Clusters" --> Pricing_Cluster & Inv_Cluster & FX_Cluster & Order_Cluster & AdaptA & AdaptB & AdaptC;
    Pricing_Cluster & Inv_Cluster & FX_Cluster & Order_Cluster & AdaptA & AdaptB & AdaptC -- "Fetch Secrets/Config via MCP" --> SecretsAgent;
    
    %% --- Final Style Applications ---
    class OrdersDB,HistDB db;
    class ExtA,ExtB,ExtC external;
    class EventHub messaging;
    class Prometheus,Grafana,Loki external;
    class CICD,SecretsAgent external;
~~~


~~~mermaid
graph TD
%% === Layers ===
subgraph User_Layer["User Interface Layer"]
UI["Procurement Dashboard (Vue.js SPA)"]
end

subgraph API_Security["API & Security Layer"]
APIGW["API Gateway (NGINX+Lua)"]
Auth["Auth Service (Keycloak)"]
Cache["Edge Cache (Redis)"]
UI -->|HTTPS| APIGW
APIGW --> Auth
APIGW <--> Cache
end

subgraph Orchestration_Core["Orchestration Core"]
OrchClient["Aggregator Client (PHP)"]
Orchestrator["PipelineOrchestrator (C Native)"]
APIGW --> OrchClient
OrchClient --> Orchestrator
end

subgraph Business_Services["Business Microservices"]
PricingSvc["Pricing Aggregator Service"]
InventorySvc["Inventory Service"]
FXSvc["FX Rate Service"]
OrderSvc["Purchase-Order Service"]
NotifySvc["Notification Service"]
Orchestrator --> PricingSvc
Orchestrator --> InventorySvc
Orchestrator --> FXSvc
Orchestrator --> OrderSvc
Orchestrator --> NotifySvc
end

subgraph Supplier_Adapters["Supplier Adapter Cluster"]
AdaptA["Supplier-A Adapter"]
AdaptB["Supplier-B Adapter"]
AdaptC["Supplier-C Adapter"]
AdaptD["Supplier-D Adapter"]
PricingSvc --> AdaptA & AdaptB & AdaptC & AdaptD
end

subgraph External_APIs["External Supplier APIs"]
ExtA["Supplier-A API"]
ExtB["Supplier-B API"]
ExtC["Supplier-C API"]
ExtD["Supplier-D API"]
AdaptA -->|REST| ExtA
AdaptB -->|REST| ExtB
AdaptC -->|REST| ExtC
AdaptD -->|REST| ExtD
end

subgraph Data_Stores["Data Stores"]
OrdersDB["Orders DB (PostgreSQL)"]
HistDB["Historical Prices (TimescaleDB)"]
Cache -->|TTL 5m| PricingSvc
PricingSvc --> HistDB
OrderSvc --> OrdersDB
end

subgraph Messaging_Observability["Messaging & Observability"]
Kafka["Event Bus (Kafka)"]
Prom["Metrics (Prometheus)"]
Grafana["Dashboards (Grafana)"]
Loki["Logs (Loki)"]
PricingSvc --> Kafka
OrderSvc --> Kafka
NotifySvc --> Kafka
PricingSvc --> Prom
OrderSvc --> Prom
Prom --> Grafana
OrchClient --> Loki
AdaptA --> Loki
AdaptB --> Loki
AdaptC --> Loki
AdaptD --> Loki
end

subgraph CICD_Secrets["CI/CD & Secrets"]
CI["CI/CD (GitLab)"]
Vault["Secrets Vault"]
Consul["Config Consul"]
CI --> APIGW
CI --> Orchestrator
CI --> PricingSvc & InventorySvc & FXSvc & OrderSvc & NotifySvc & AdaptA & AdaptB & AdaptC & AdaptD
OrchClient --> Vault
AdaptA --> Vault
OrchClient --> Consul
end

~~~


~~~mermaid
graph TD
    %% --- Style Definitions ---
    classDef user fill:#ef6c00,color:white
    classDef orchestrator fill:#0097a7,stroke:#006064,stroke-width:4px,color:white
    classDef agent_cluster fill:#512da8,color:white,stroke-width:2px
    classDef worker fill:#d1c4e9,stroke:#512da8,stroke-width:1px,color:#311b92
    classDef logic fill:#b2ebf2,stroke:#0097a7,stroke-width:2px,color:#004d40
    classDef model fill:#fce4ec,stroke:#c2185b,stroke-width:1px,color:#880e4f
    classDef db fill:#ede7f6,stroke:#4527a0,stroke-width:2px,color:#311b92
    classDef external fill:#424242,color:white
    classDef trade fill:#c2185b,color:white,stroke-width:2px


    %% --- 1. User & Orchestrator Client ---
    subgraph "TIER 1: User & Orchestration Client"
        A("fa:fa-user Trader at<br/>Procurement Dashboard<br/>-");
        B["fa:fa-play-circle Orchestrator Client<br/>(run_aggregator_pipeline.php)<br/>-"];
        C{"fa:fa-cogs PipelineOrchestrator<br/>(C-Native Engine)<br/>-"};
        A -- "Request price for SKU & Settlement Date" --> B;
        B -- "Executes Pipeline Definition" --> C;
    end
    
    %% --- 2. Parallel Data Ingestion & Hedging Agents ---
    subgraph "TIER 2: Data Ingestion & Financial Agents (MCP Clusters)"
        direction LR

        subgraph "Supplier Agents"
            S_Cluster["fa:fa-server Quicpro\Cluster"];
            S_Worker["fa:fa-cogs Supplier Agent<br/>Worker<br/>-"];
            S_MCP("fa:fa-network-wired MCP Server");
            S_Cluster --> S_Worker --> S_MCP;
        end
        
        subgraph "FX Data Agent"
            FX_Data_Cluster["fa:fa-server Quicpro\Cluster"];
            FX_Data_Worker["fa:fa-cogs FX Data Agent<br/>Worker<br/>-"];
            FX_Data_MCP("fa:fa-network-wired MCP Server");
            FX_Data_Cluster --> FX_Data_Worker --> FX_Data_MCP;
        end

        subgraph "FX Trading Agent (HEDGING)"
            FX_Trade_Cluster["fa:fa-server Quicpro\Cluster"];
            FX_Trade_Worker["fa:fa-cogs FX Trading Agent<br/>Worker<br/>-"];
            FX_Trade_MCP("fa:fa-network-wired MCP Server");
            FX_Trade_Cluster --> FX_Trade_Worker --> FX_Trade_MCP;
        end
    end

    %% --- 3. Analytics & Prediction Engine ---
    subgraph "TIER 3: Analytics & Prediction Engine (MCP Cluster)"
        Analytics_Cluster["fa:fa-server Quicpro\Cluster"];
        subgraph "Analytics Worker Process"
            A_Worker["fa:fa-cogs Analytics Agent<br/>Worker<br/>-"];
            A_Logic{"fa:fa-calculator Analysis &<br/>Prediction Logic<br/>-"};
            A_Model[("fa:fa-brain Price Prediction Model")];
            A_DB_Client[("fa:fa-database-alt Historical DB Client")];
            A_Logic --> A_Model & A_DB_Client;
        end
        Analytics_Cluster --> A_Worker;
    end
    
    %% --- 4. External & Persistence Layers ---
    subgraph "TIER 4: External Systems & Data Stores"
        direction LR
        Ext_Suppliers(fa:fa-globe-americas Supplier APIs);
        Ext_FX_Data(fa:fa-dollar-sign FX Rate APIs);
        Ext_FX_Trade("fa:fa-handshake Forex Trading<br/>Platform (FIX API)<br/>-");
        Hist_DB[("fa:fa-server-rack Historical Price DB<br/>(TimescaleDB)<br/>-")];
    end

    %% --- Data Flows ---
    C -- "Parallel MCP Call<br/>GetPrice(SKU)<br/>-" --> S_MCP;
    C -- "Parallel MCP Call<br/>GetFXRate(EURUSD, date)<br/>-" --> FX_Data_MCP;
    
    S_MCP -- HTTPS --> Ext_Suppliers;
    FX_Data_MCP -- HTTPS --> Ext_FX_Data;
    
    S_MCP -- "Live Prices" --> C;
    FX_Data_MCP -- "Forward FX Rate" --> C;
    
    C -- "Fan-In: Send all results<br/>to Analytics Agent (MCP)<br/>-" --> A_Worker;
    A_DB_Client -- "SQL Query" --> Hist_DB;
    A_Worker -- "Enriched & Predicted Prices" --> C;
    
    C -- "Decision: Hedge Forex?<br/>(Based on results & policy)<br/>-" --> FX_Trade_MCP;
    FX_Trade_MCP -- "Execute Forward Contract<br/>(FIX Protocol over TCP/QUIC)<br/>-" --> Ext_FX_Trade;
    FX_Trade_MCP -- "Hedge Confirmation" --> C;

    C -- "Final consolidated offer<br/>(with hedge confirmation)<br/>-" --> B;
    B -- "Displays Best Offer" --> A;

    %% --- Apply Styles ---
    class A,B user;
    class C orchestrator;
    class S_Cluster,S_Worker,S_MCP,FX_Data_Cluster,FX_Data_Worker,FX_Data_MCP,Analytics_Cluster,A_Worker agent_cluster;
    class A_Logic,A_Model model;
    class Hist_DB,A_DB_Client db;
    class Ext_Suppliers,Ext_FX_Data,Ext_FX_Trade external;
    class FX_Trade_Cluster,FX_Trade_Worker,FX_Trade_MCP trade;
~~~

~~~mermaid
graph TD
    %% --- Style Definitions ---
    classDef user fill:#ef6c00,color:white
    classDef orchestrator fill:#0097a7,stroke:#006064,stroke-width:4px,color:white
    classDef agent fill:#512da8,color:white,stroke-width:2px
    classDef logic fill:#b2ebf2,stroke:#0097a7,stroke-width:2px,color:#004d40
    classDef external fill:#424242,color:white

    subgraph "User & Application Layer"
        A("fa:fa-user Procurement Manager<br/>at Dashboard UI<br> - ");
        B["fa:fa-play-circle Orchestrator Client<br/>(run_aggregator_pipeline.php)<br> - "];
        C[("fa:fa-php Aggregator Function<br/>(find_best_price)<br> - ")]
        A -- "Submits SKU for lookup" --> B;
    end
    
    subgraph "quicpro_async Backend"
        D{{"fa:fa-cogs PipelineOrchestrator<br/>(C-Native Engine)<br> - "}};
        
        subgraph "Supplier Agents (MCP Microservices)<br> - "
            direction LR
            SA["fa:fa-server Supplier A<br/>Agent<br> - "];
            SB["fa:fa-server Supplier B<br/>Agent<br> - "];
            SC["fa:fa-server Supplier C<br/>Agent<br> - "];
            SD["fa:fa-server Supplier D<br/>Agent<br> - "];
        end
    end

    subgraph "External Systems"
        direction LR
        EA(fa:fa-globe Supplier A API);
        EB(fa:fa-globe Supplier B API);
        EC(fa:fa-globe Supplier C API);
        ED(fa:fa-globe Supplier D API);
    end

    %% --- Data Flows ---
    B -- "Executes Pipeline" --> D;
    
    D -- "MCP Fan-Out<br/>(Concurrent Requests)<br/>-" --> SA;
    D -- "..." --> SB;
    D -- "..." --> SC;
    D -- "..." --> SD;

    SA -- "HTTPS" --> EA;
    SB -- "HTTPS" --> EB;
    SC -- "HTTPS" --> EC;
    SD -- "HTTPS" --> ED;

    SA & SB & SC & SD -- "MCP Responses<br/>(Prices & Stock)<br/>-" --> D;
    
    D -- "Fan-In: Passes results<br/>when finish condition is met<br> - " --> C;
    C -- "Returns Best Offer" --> B;
    
    B -- "Displays Final Result" --> A;

    %% --- Apply Styles ---
    class A,B user;
    class C logic;
    class D orchestrator;
    class SA,SB,SC,SD agent;
    class EA,EB,EC,ED external;
~~~
