~~~mermaid
graph TD
    %% --- Style Definitions ---
    classDef main_app fill:#d32f2f,color:white
    classDef core_comp fill:#ef6c00,color:white
    classDef network_layer fill:#0097a7,stroke:#006064,stroke-width:2px,color:white
    classDef protocol_handler fill:#42a5f5,stroke:#2196f3,stroke-width:1px,color:black
    classDef module_api fill:#ab47bc,stroke:#8e24aa,stroke-width:1px,color:white
    classDef routing_logic fill:#c2185b,color:white
    classDef external_api fill:#6a1b9a,color:white
    classDef call_flow fill:#b2ebf2,stroke:#0097a7,stroke-width:2px,color:#004d40
    classDef storage fill:#fbc02d,color:black
    classDef ai_ml fill:#8bc34a,color:white


    %% --- Quicpro PBX Server (PHP + quicpro_async) ---
    subgraph "Quicpro PBX Server (PHP + quicpro_async)"
        direction TB

        %% Main Application Entry Point
        MainApp["fa:fa-server Main PBX Application<br/>(PHP Orchestrator, Event Loop)"]:::main_app;

        %% Core PBX Components
        CallManager["fa:fa-phone-volume Call Manager<br/>(Call State, Bridging, Conferencing, RTP Proxy)"]:::core_comp;
        DialplanEngine["fa:fa-code-branch Dialplan Engine<br/>(Call Routing, PHP Callbacks, AGI-like Execution)"]:::routing_logic;
        ExtensionRegistry["fa:fa-id-card Extension/User Registry<br/>(User/Device DB, Authentication, Presence)"]:::core_comp;
        FeatureServer["fa:fa-star Feature Server<br/>(Voicemail, IVR, Ring Groups, Queues)"]:::core_comp;
        BillingEngine["fa:fa-receipt Billing Engine<br/>(CDR Generation, Rating, Quotas)"]:::core_comp;
        RecordingManager["fa:fa-microphone-alt Recording Manager<br/>(Call Recording Control, Transcoding)"]:::core_comp;


        %% Network & Protocol Layers (Built with quicpro_async)
        TCPServerStack["fa:fa-network-wired TCP Server Stack<br/>(HTTP/1.1, HTTP/2, SIP/TCP, WebSocket)"]:::network_layer;
        UDPServerStack["fa:fa-network-wired UDP Server Stack<br/>(HTTP/3, SIP/UDP, RTP/SRTP, DNS, WebTransport)"]:::network_layer;

        SIPProtocol["fa:fa-sitemap SIP Protocol Handler<br/>(Parser, UA State, Registrar, Proxy)"]:::protocol_handler;
        RTPMediaStack["fa:fa-exchange-alt RTP/SRTP Media Stack<br/>(Audio/Video Handling, Codecs, Transcoding)"]:::protocol_handler;
        HTTPClientServerStack["fa:fa-globe HTTP Client/Server Stack<br/>(HTTP/1.1, HTTP/2 via libcurl)"]:::protocol_handler;
        HTTP3QUICStack["fa:fa-bolt HTTP/3 over QUIC Stack<br/>(Client/Server via quiche)"]:::protocol_handler;
        WebSocketProtocol["fa:fa-websocket WebSocket Protocol<br/>(SIP-over-WS, WebRTC Signaling)"]:::protocol_handler;
        DNSServerStack["fa:fa-dns DNS Server<br/>(Smart DNS, Service Discovery, DNSSEC)"]:::protocol_handler;
        WebTransportStack["fa:fa-sitemap WebTransport Protocol<br/>(Real-time Data, QUIC)"]:::protocol_handler;


        %% Internal Infrastructure Modules (quicpro_async)
        ConfigModule["fa:fa-cogs Configuration Module<br/>(Unified INI/PHP Config, Dynamic Updates)"]:::module_api;
        TLSModule["fa:fa-lock TLS/SSL Module<br/>(Cert Mgmt, Handshakes, mTLS, OCSP)"]:::module_api;
        ClusterModule["fa:fa-boxes Cluster Supervisor<br/>(Worker Mgmt, Auto-Scaling, HA)"]:::module_api;
        NativeObjectStore["fa:fa-database Native Object Store<br/>(Recordings, Voicemail, Blob Storage)"]:::storage;
        TelemetryModule["fa:fa-chart-line OpenTelemetry Module<br/>(Metrics, Tracing, Logs, Health Checks)"]:::module_api;
        IIBINModule["fa:fa-code IIBIN Serialization<br/>(Binary Protocol Codec, Schema Registry)"]:::module_api;
        MCPModule["fa:fa-project-diagram MCP Module<br/>(Model Transfer Protocol, Microservice Comm.)"]:::module_api;
        PipelineOrchestrator["fa:fa-sitemap Pipeline Orchestrator<br/>(Workflow Automation, Tool Invocation)"]:::routing_logic;
        SemanticGeometryModule["fa:fa-cubes Semantic Geometry<br/>(Vector/Spatial Processing, AI Integration)"]:::ai_ml;
        HighPerfComputeAI["fa:fa-brain High Perf Compute & AI<br/>(DataFrame, GPU Bindings, CUDA/ROCm)"]:::ai_ml;
        SmartContractsModule["fa:fa-handshake Smart Contracts<br/>(DLT Integration, Identity, Billing)"]:::module_api;
        RouterLoadBalancer["fa:fa-route Router & Load Balancer<br/>(Internal Routing, Backend Discovery)"]:::routing_logic;
        AdminAPI["fa:fa-terminal Admin API<br/>(Remote Management, Monitoring)"]:::external_api;
        SSHOverQUIC["fa:fa-shield-alt SSH over QUIC<br/>(Secure Remote Management, Node Provisioning)"]:::external_api;


        %% External / Outbound Integration (via quicpro_async Client Capabilities)
        ExternalPSTNSIPGateway["fa:fa-cloud External PSTN/SIP Gateway<br/>(SIP Trunk Providers, Carriers)"]:::external_api;
        ExternalCRMAIServices["fa:fa-cloud-meatball External CRM/AI Services<br/>(CRM Sync, Voicebots, Transcriptions)"]:::external_api;
        CloudProviderAPI["fa:fa-cloud-upload-alt Cloud Provider API<br/>(Hetzner, AWS, Azure for Auto-Scaling)"]:::external_api;


        %% Internal Data Flow and Control
        MainApp --> TCPServerStack & UDPServerStack;
        MainApp --> ConfigModule & ClusterModule & TelemetryModule;
        MainApp --> DialplanEngine & CallManager;

        TCPServerStack --> SIPProtocol & HTTPClientServerStack & WebSocketProtocol;
        UDPServerStack --> SIPProtocol & RTPMediaStack & HTTP3QUICStack & DNSServerStack & WebTransportStack;

        SIPProtocol -- "Signaling" --> CallManager;
        RTPMediaStack -- "Media" --> CallManager;
        WebSocketProtocol -- "Signaling" --> CallManager;
        WebTransportStack -- "Real-time Data" --> CallManager;

        CallManager --> DialplanEngine;
        CallManager --> RTPMediaStack;
        CallManager --> FeatureServer;
        CallManager --> BillingEngine;
        CallManager --> RecordingManager;

        DialplanEngine --> ExtensionRegistry;
        DialplanEngine --> SIPProtocol -- "Outbound Signaling" --> ExternalPSTNSIPGateway;
        DialplanEngine --> HTTPClientServerStack -- "Outbound HTTP" --> ExternalCRMAIServices;
        DialplanEngine --> HTTP3QUICStack -- "Outbound HTTP/3" --> ExternalCRMAIServices;
        DialplanEngine --> PipelineOrchestrator;

        FeatureServer --> ExtensionRegistry;

        RecordingManager --> NativeObjectStore;

        ConfigModule --> TLSModule & ClusterModule & NativeObjectStore & TelemetryModule & IIBINModule & MCPModule & PipelineOrchestrator & SemanticGeometryModule & HighPerfComputeAI & SmartContractsModule & RouterLoadBalancer & AdminAPI & SSHOverQUIC;

        TelemetryModule -- "Metrics/Logs" --> CloudProviderAPI; 

        ClusterModule -- "Provisioning" --> CloudProviderAPI; 
        ClusterModule -- "Remote Config" --> SSHOverQUIC; 

        MCPModule -- "Internal Service Comm" --> HTTPClientServerStack & HTTP3QUICStack; 
        PipelineOrchestrator -- "Tool Invocation" --> MCPModule;
        PipelineOrchestrator -- "AI/ML Integration" --> HighPerfComputeAI & SemanticGeometryModule;

        HighPerfComputeAI --> SemanticGeometryModule;
        HighPerfComputeAI --> NativeObjectStore; 

        SmartContractsModule -- "Blockchain Interaction" --> HTTPClientServerStack;
        SmartContractsModule --> IIBINModule; 

        DNSServerStack -- "Service Discovery" --> RouterLoadBalancer; 
        RouterLoadBalancer -- "Routing Decisions" --> TCPServerStack & UDPServerStack; 

        AdminAPI -- "Management Access" --> HTTPClientServerStack & HTTP3QUICStack; 
        SSHOverQUIC -- "Remote Shell/Config" --> AdminAPI; 


        %% Call Flow Example
        UserPhone["fa:fa-tty User Phone<br/>(SIP/WebRTC Client)"]:::call_flow;
        UserPhone -- "SIP REGISTER (UDP)" --> UDPServerStack;
        UserPhone -- "SIP INVITE (TCP)" --> TCPServerStack;
        UDPServerStack & TCPServerStack --> SIPProtocol;
        SIPProtocol --> ExtensionRegistry -- "Auth" --> ExtensionRegistry;
        SIPProtocol --> CallManager;
        CallManager --> DialplanEngine;
        DialplanEngine -- "Internal Route" --> ExtensionRegistry;
        DialplanEngine -- "External Route" --> ExternalPSTNSIPGateway;
        CallManager -- "Setup Media" --> RTPMediaStack;
        RTPMediaStack -- "Audio/Video" --> UserPhone;

    end
~~~


~~~mermaid
graph TD
    %% --- Style Definitions ---
    classDef main_app fill:#d32f2f,color:white
    classDef core_comp fill:#ef6c00,color:white
    classDef network_layer fill:#0097a7,stroke:#006064,stroke-width:2px,color:white
    classDef protocol_handler fill:#42a5f5,stroke:#2196f3,stroke-width:1px,color:black
    classDef module_api fill:#ab47bc,stroke:#8e24aa,stroke-width:1px,color:white
    classDef routing_logic fill:#c2185b,color:white
    classDef external_api fill:#6a1b9a,color:white
    classDef call_flow fill:#b2ebf2,stroke:#0097a7,stroke-width:2px,color:#004d40
    classDef storage fill:#fbc02d,color:black


    %% --- PBX Server Main Application (PHP + quicpro_async) ---
    subgraph "Custom PBX Server Application (PHP + quicpro_async)"
        direction TB

        %% Main Application Entry Point
        MainApp["fa:fa-server Main PBX Application<br/>(PHP Entry Point)"];

        %% Core Components
        CallManager["fa:fa-phone-volume Call Manager<br/>(Manages Call State, Bridging)<br/>-"];
        DialplanEngine["fa:fa-code-branch Dialplan Engine<br/>(Call Routing Logic, Contexts)<br/>-"];
        ExtensionRegistry["fa:fa-id-card Extension Registry<br/>(User/Device DB, Authentication)<br/>-"];

        %% Network & Protocol Layers (Built with quicpro_async)
        TCPServer["fa:fa-network-wired TCP Server<br/>(Listen for SIP/RTP/HTTP/Websocket)"];
        UDPServer["fa:fa-network-wired UDP Server<br/>(Listen for SIP/RTP/QUIC/DNS)"];

        SIPStack["fa:fa-sitemap SIP Protocol Stack<br/>(Parser, State Machine, Registrar)<br/>-"];
        RTPStack["fa:fa-exchange-alt RTP/SRTP Media Stack<br/>(Audio/Video Handling, Codecs)<br/>-"];
        HTTPStack["fa:fa-globe HTTP/1.1 & HTTP/2 Stack<br/>(Client & Server with libcurl)<br/>-"];
        HTTP3Stack["fa:fa-bolt HTTP/3 over QUIC Stack<br/>(Client & Server with quiche)<br/>-"];
        WebSocketStack["fa:fa-websocket WebSocket Protocol<br/>(Client & Server API)<br/>-"];
        DNSServer["fa:fa-dns DNS Server<br/>(Service Discovery, SRV lookups)<br/>-"];

        %% Internal Infrastructure Modules (quicpro_async)
        ConfigModule["fa:fa-cogs Configuration Module<br/>(Unified INI/PHP Config)"];
        TLSModule["fa:fa-lock TLS/SSL Module<br/>(Certificate Mgmt, Handshakes)"];
        ClusterModule["fa:fa-boxes Cluster Supervisor<br/>(Worker Mgmt, Scaling)"];
        StorageModule["fa:fa-database Native Object Store<br/>(Recording, Voicemail)"];
        TelemetryModule["fa:fa-chart-line OpenTelemetry Module<br/>(Metrics, Tracing, Logs)"];
        IIBINModule["fa:fa-code IIBIN Serialization<br/>(Binary Protocol Codec)"];

        %% External / Outbound Integration (via quicpro_async Client Capabilities)
        ExternalGateway["fa:fa-cloud External PSTN/SIP Gateway<br/>(e.g., SIP Trunk Provider)"];
        ExternalAPIs["fa:fa-cloud-meatball External APIs<br/>(e.g., CRM, AI Services)"];

        %% Interconnections
        MainApp --> CallManager;
        MainApp --> DialplanEngine;
        MainApp --> ExtensionRegistry;

        MainApp --> TCPServer;
        MainApp --> UDPServer;

        TCPServer --> SIPStack;
        TCPServer --> HTTPStack;
        TCPServer --> WebSocketStack;

        UDPServer --> SIPStack;
        UDPServer --> RTPStack;
        UDPServer --> HTTP3Stack;
        UDPServer --> DNSServer;

        SIPStack -- "Call Signaling" --> CallManager;
        RTPStack -- "Media Streams" --> CallManager;

        CallManager --> DialplanEngine;
        DialplanEngine --> ExtensionRegistry;
        
        DialplanEngine -- "Outbound Calls" --> SIPStack;
        DialplanEngine -- "Advanced Routing" --> ExternalGateway;
        DialplanEngine -- "Service Integration" --> ExternalAPIs;

        HTTPStack -- "WebRTC Signaling/APIs" --> CallManager;
        HTTP3Stack -- "WebRTC Signaling/APIs" --> CallManager;
        WebSocketStack -- "WebRTC Signaling/APIs" --> CallManager;

        CallManager -- "Recordings, Voicemail" --> StorageModule;
        CallManager --> TelemetryModule;
        DialplanEngine --> TelemetryModule;

        SIPStack --> TLSModule;
        HTTPStack --> TLSModule;
        HTTP3Stack --> TLSModule;
        WebSocketStack --> TLSModule;
        ExternalGateway --> TLSModule;
        ExternalAPIs --> TLSModule;

        MainApp --> ConfigModule;
        ConfigModule --> TLSModule & ClusterModule & StorageModule & TelemetryModule & IIBINModule;
        
        ExternalAPIs --> IIBINModule; 

        ClusterModule --> MainApp;

        %% Call Flow Example
        UserPhone("fa:fa-tty User Phone<br/>(SIP/WebRTC)");
        UserPhone -- "SIP REGISTER / INVITE" --> TCPServer;
        UserPhone -- "SIP REGISTER / INVITE" --> UDPServer;
        TCPServer & UDPServer --> SIPStack;
        SIPStack --> CallManager;
        CallManager --> DialplanEngine;
        DialplanEngine -- "Call flow decision" --> CallManager;
        CallManager -- "Set up media" --> RTPStack;
        RTPStack -- "Audio" --> UserPhone;

        ExternalGateway -- "SIP Signaling" --> SIPStack;
        ExternalGateway -- "RTP Media" --> RTPStack;

    end

    %% Apply Styles
    class MainApp main_app;
    class CallManager,DialplanEngine,ExtensionRegistry core_comp;
    class TCPServer,UDPServer network_layer;
    class SIPStack,RTPStack,HTTPStack,HTTP3Stack,WebSocketStack,DNSServer protocol_handler;
    class ConfigModule,TLSModule,ClusterModule,StorageModule,TelemetryModule,IIBINModule module_api;
    class ExternalGateway,ExternalAPIs external_api;
    class UserPhone call_flow;
~~~


~~~mermaid
graph TD
%% --- Style Definitions ---
    classDef external fill:#424242,color:white
    classDef pbx fill:#d32f2f,color:white
    classDef gateway fill:#0097a7,stroke:#006064,stroke-width:4px,color:white
    classDef cluster_supervisor fill:#006064,color:white
    classDef worker_process fill:#b2ebf2,stroke:#0097a7,stroke-width:2px,color:#004d40
    classDef mcp_server fill:#fff,stroke:#0097a7,stroke-width:1px,color:#004d40
    classDef ai_model fill:#fce4ec,stroke:#c2185b,stroke-width:2px,color:#880e4f
    classDef human_phone fill:#ef6c00,color:white
    classDef knowledge fill:#f3e5f5,stroke:#880e4f,stroke-width:2px,color:#311b92
    classDef router_agent fill:#c2185b,color:white,stroke-width:2px

%% --- Call Origination ---
    subgraph "External & Home Network"
        A("fa:fa-phone-alt External Caller");
        C("fa:fa-tty User Handset<br/>(IP/DECT Phone)<br/>-");
        A -- "Phone Call" --> Asterisk;
    end

%% --- Voice Gateway (The Orchestrator) ---
    subgraph "Voice Gateway Application (PHP + quicpro_async)"
        direction TB
        Asterisk["fa:fa-server Asterisk PBX<br/>(Core Call Routing)<br/>-"];
        AMI_Interface["fa:fa-code Asterisk AMI Interface<br/>(PHP control of Asterisk calls)<br/>-"];
        AGI_Script["fa:fa-code Asterisk AGI Script<br/>(PHP for In-Call Logic)<br/>-"];
        Logic_Core{"fa:fa-cogs Conversation Logic Core<br/>(Orchestrates AI agents via MCP)<br/>-"};
        RTP_Handler["fa:fa-exchange-alt RTP Media Handler<br/>(Manages raw audio streams with Asterisk Integration)<br/>-"];

        Asterisk -- "AMI Events" --> AMI_Interface;
        AMI_Interface -- "Control Commands" --> Asterisk;
        Asterisk -- "Call Flow Trigger (e.g. Dialplan)" --> AGI_Script;
        AGI_Script -- "Execute Commands & Get Input" --> Asterisk;
        AGI_Script -- "Audio Stream<br/>(via FastAGI/MixMonitor)" --> RTP_Handler; 

        AMI_Interface -- "Call State Updates" --> Logic_Core;
        AGI_Script -- "Initiates Logic" --> Logic_Core;
        RTP_Handler -- "Audio In/Out" --> Logic_Core;
    end

%% --- AI Microservice Clusters ---
subgraph "AI Inference Backend (Decoupled & Scalable)"
direction TB

subgraph "STT Cluster"
STT_Supervisor["fa:fa-server Quicpro\Cluster"];
STT_Worker["fa:fa-cogs STT Worker Process<br/>-"];
subgraph "Inside STT Worker"
STT_MCP["MCP Server"];
Whisper_Model("fa:fa-microphone-alt Whisper Model");
STT_MCP -.-> Whisper_Model;
end
STT_Supervisor -->|"fork()"| STT_Worker;
STT_Worker --> STT_MCP;
end

subgraph "Pluggable LLM Subsystem"
LLM_Router["fa:fa-route LLM Router Agent<br/>(Selects Model/Strategy)<br/>-"];

subgraph "Local Mixture of Experts (MoE) Cluster"
direction TB
MoE_Router["fa:fa-cogs MoE Gating Agent<br/>(Selects Experts)<br/>-"];
subgraph "Expert Model Shards (GPU Clusters)"
direction LR
Expert_Code["fa:fa-server Expert Cluster A<br/>(Code Analysis)<br/>-"];
Expert_Logic["fa:fa-server Expert Cluster B<br/>(Logic & Reasoning)<br/>-"];
Expert_N["..."];
end
MoE_Aggregator["fa:fa-wave-square MoE Aggregator Agent<br/>(Combines Expert Outputs)<br/>-"];
MoE_Router --> Expert_Code & Expert_Logic & Expert_N;
Expert_Code & Expert_Logic & Expert_N --> MoE_Aggregator;
end

subgraph "Cloud LLM Adapters (MCP Workers)"
OpenAI_Worker["fa:fa-cogs OpenAI GPT-4o Worker"];
Anthropic_Worker["fa:fa-cogs Anthropic Claude 3.5 Worker"];
end

LLM_Router -- "Routes to Local MoE" --> MoE_Router;
LLM_Router -- "Routes to Cloud" --> OpenAI_Worker & Anthropic_Worker;
MoE_Aggregator -- "Final Local Response" --> LLM_Router;
end

subgraph "Pluggable Text-to-Speech (TTS) Subsystem"
TTS_Router["fa:fa-route TTS Router Agent"];
subgraph "Local & Cloud TTS Engines"
TTS_Piper["fa:fa-cogs Piper TTS Worker"];
TTS_Bark["fa:fa-cogs Bark TTS Worker"];
TTS_Cloud["fa:fa-cogs Cloud API Workers<br/>(Google, AWS Polly, etc)<br/>-"];
end
TTS_Router --> TTS_Piper & TTS_Bark & TTS_Cloud;
end

subgraph "GraphRAG & Caching"
RAG_Cache[("fa:fa-database GraphRAG Cache Agent<br/>(Redis or similar)<br/>-")];
KG[("fa:fa-database Knowledge Graph<br/>(Neo4j)<br/>-")];
RAG_Cache -- "Cache Miss" --> KG;
end
end

%% --- DATA FLOWS ---
Asterisk -- "SIP Invite / RTP Audio Stream" --> A & C;
AMI_Interface -- "Call events" --> Logic_Core;
AGI_Script -- "Audio In/Out" --> Logic_Core;

Logic_Core -- "MCP Call<br/>STT.transcribe(audio)<br/>-" --> STT_MCP;
STT_MCP -- "MCP Response<br/>{text: '...'}<br/>-" --> Logic_Core;

Logic_Core -- "MCP Call<br/>LLM.decide(text, context)<br/>-" --> LLM_Router;
LLM_Router -- "MCP Call to get context" --> RAG_Cache;
RAG_Cache -- "Returns cached or fresh context" --> LLM_Router;
LLM_Router -- "Forwards prompt with context" --> MoE_Router;

LLM_Router -- "MCP Response<br/>{action: 'speak' or 'escalate' or 'transfer'}<br/>-" --> Logic_Core;

Logic_Core -- "IF action == 'speak'<br/>MCP Call: TTS.synthesize(text)<br/>-" --> TTS_Router;
TTS_Router -- "MCP Response<br/>{audio: '...'}<br/>-" --> Logic_Core;
Logic_Core -- "Generated Audio" --> RTP_Handler;
RTP_Handler -- "RTP Audio Stream<br/>(AI's Voice)<br/>-" --> Asterisk;

Logic_Core -- "IF action == 'transfer'<br/>AMI Call: TransferCall(channel, extension)<br/>-" --> AMI_Interface;
Logic_Core -- "IF action == 'dial'<br/>AMI Call: Originate(channel, context, extension)<br/>-" --> AMI_Interface;

%% Apply Styles
class A,C external;
class Asterisk pbx;
class AMI_Interface,AGI_Script,Logic_Core,RTP_Handler gateway;
class STT_Supervisor,LLM_Supervisor,TTS_Supervisor cluster_supervisor;
class STT_Worker,LLM_Worker,TTS_Worker,MoE_Aggregator,OpenAI_Worker,Anthropic_Worker worker_process;
class STT_MCP,LLM_MCP,TTS_MCP,MoE_Router mcp_server;
class Whisper_Model,LLM_Model,Piper_Model,TTS_Cloud,Expert_Code,Expert_Logic,Expert_N ai_model;
class C human_phone;
class TTS_Router,LLM_Router router_agent;
class KG,RAG_Cache knowledge;
~~~

~~~mermaid
graph TD
%% --- Style Definitions ---
    classDef external fill:#424242,color:white
    classDef pbx fill:#d32f2f,color:white
    classDef gateway fill:#0097a7,stroke:#006064,stroke-width:4px,color:white
    classDef cluster_supervisor fill:#006064,color:white
    classDef worker_process fill:#b2ebf2,stroke:#0097a7,stroke-width:2px,color:#004d40
    classDef mcp_server fill:#fff,stroke:#0097a7,stroke-width:1px,color:#004d40
    classDef ai_model fill:#fce4ec,stroke:#c2185b,stroke-width:2px,color:#880e4f
    classDef human_phone fill:#ef6c00,color:white
    classDef knowledge fill:#f3e5f5,stroke:#880e4f,stroke-width:2px,color:#311b92
    classDef router_agent fill:#c2185b,color:white,stroke-width:2px


%% --- Call Origination ---
    subgraph "External & Home Network"
        A("fa:fa-phone-alt External Caller");
        B["fa:fa-server VoIP Gateway / PBX<br/>(e.g., Fritz!Box, Asterisk)<br/>-"];
        C("fa:fa-tty User Handset<br/>(IP/DECT Phone)<br/>-");
        A -- "Phone Call" --> B;
    end

%% --- Voice Gateway (The Orchestrator) ---
    subgraph "Voice Gateway Application (PHP + quicpro_async)"
        direction TB
        SIP_Handler["fa:fa-headset SIP Agent<br/>(Call Control)<br/>-"];
        RTP_Handler["fa:fa-exchange-alt RTP Media Handler<br/>(Manages raw audio streams)<br/>-"];
        Logic_Core{"fa:fa-cogs Conversation Logic Core<br/>(Orchestrates AI agents via MCP)<br/>-"};

        SIP_Handler -- "Call events" --> Logic_Core;
        RTP_Handler -- "Audio In/Out" --> Logic_Core;
    end

%% --- AI Microservice Clusters ---
subgraph "AI Inference Backend (Decoupled & Scalable)"
direction TB

subgraph "STT Cluster"
STT_Supervisor["fa:fa-server Quicpro\Cluster"];
STT_Worker["fa:fa-cogs STT Worker Process<br/>-"];
subgraph "Inside STT Worker"
STT_MCP["MCP Server"];
Whisper_Model("fa:fa-microphone-alt Whisper Model");
STT_MCP -.-> Whisper_Model;
end
STT_Supervisor -->|"fork()"| STT_Worker;
STT_Worker --> STT_MCP;
end

subgraph "Pluggable LLM Subsystem"
LLM_Router["fa:fa-route LLM Router Agent<br/>(Selects Model/Strategy)<br/>-"];

subgraph "Local Mixture of Experts (MoE) Cluster"
direction TB
MoE_Router["fa:fa-cogs MoE Gating Agent<br/>(Selects Experts)<br/>-"];
subgraph "Expert Model Shards (GPU Clusters)"
direction LR
Expert_Code["fa:fa-server Expert Cluster A<br/>(Code Analysis)<br/>-"];
Expert_Logic["fa:fa-server Expert Cluster B<br/>(Logic & Reasoning)<br/>-"];
Expert_N["..."];
end
MoE_Aggregator["fa:fa-wave-square MoE Aggregator Agent<br/>(Combines Expert Outputs)<br/>-"];
MoE_Router --> Expert_Code & Expert_Logic & Expert_N;
Expert_Code & Expert_Logic & Expert_N --> MoE_Aggregator;
end

subgraph "Cloud LLM Adapters (MCP Workers)"
OpenAI_Worker["fa:fa-cogs OpenAI GPT-4o Worker"];
Anthropic_Worker["fa:fa-cogs Anthropic Claude 3.5 Worker"];
end

LLM_Router -- "Routes to Local MoE" --> MoE_Router;
LLM_Router -- "Routes to Cloud" --> OpenAI_Worker & Anthropic_Worker;
MoE_Aggregator -- "Final Local Response" --> LLM_Router;
end

subgraph "Pluggable Text-to-Speech (TTS) Subsystem"
TTS_Router["fa:fa-route TTS Router Agent"];
subgraph "Local & Cloud TTS Engines"
TTS_Piper["fa:fa-cogs Piper TTS Worker"];
TTS_Bark["fa:fa-cogs Bark TTS Worker"];
TTS_Cloud["fa:fa-cogs Cloud API Workers<br/>(Google, AWS Polly, etc)<br/>-"];
end
TTS_Router --> TTS_Piper & TTS_Bark & TTS_Cloud;
end

subgraph "GraphRAG & Caching"
RAG_Cache[("fa:fa-database GraphRAG Cache Agent<br/>(Redis or similar)<br/>-")];
KG[("fa:fa-database Knowledge Graph<br/>(Neo4j)<br/>-")];
RAG_Cache -- "Cache Miss" --> KG;
end
end

%% --- DATA FLOWS ---
B -- "SIP Invite / RTP Audio Stream" --> SIP_Handler & RTP_Handler;
Logic_Core -- "MCP Call<br/>STT.transcribe(audio)<br/>-" --> STT_MCP;
STT_MCP -- "MCP Response<br/>{text: '...'}<br/>-" --> Logic_Core;

Logic_Core -- "MCP Call<br/>LLM.decide(text, context)<br/>-" --> LLM_Router;
LLM_Router -- "MCP Call to get context" --> RAG_Cache;
RAG_Cache -- "Returns cached or fresh context" --> LLM_Router;
LLM_Router -- "Forwards prompt with context" --> MoE_Router;

LLM_Router -- "MCP Response<br/>{action: 'speak' or 'escalate'}<br/>-" --> Logic_Core;

Logic_Core -- "IF action == 'speak'<br/>MCP Call: TTS.synthesize(text)<br/>-" --> TTS_Router;
TTS_Router -- "MCP Response<br/>{audio: '...'}<br/>-" --> Logic_Core;
Logic_Core -- "Generated Audio" --> RTP_Handler;
RTP_Handler -- "RTP Audio Stream<br/>(AI's Voice)<br/>-" --> B;

Logic_Core -- "IF action == 'escalate'<br/>TR-064 Call: DialNumber()<br/>-" --> B;
B -- "Rings" --> C;

%% Apply Styles
class A,C,API_Google,API_OpenAI,API_AWS external;
class B pbx;
class SIP_Handler,RTP_Handler,Logic_Core gateway;
class STT_Supervisor,LLM_Supervisor,TTS_Supervisor cluster_supervisor;
class STT_Worker,LLM_Worker,TTS_Worker,MoE_Aggregator,OpenAI_Worker,Anthropic_Worker worker_process;
class STT_MCP,LLM_MCP,TTS_MCP,MoE_Router mcp_server;
class Whisper_Model,LLM_Model,Piper_Model,TTS_Cloud,Expert_Code,Expert_Logic,Expert_N ai_model;
class C human_phone;
class TTS_Router,LLM_Router router_agent;
class KG,RAG_Cache knowledge;
~~~


~~~mermaid
graph TD
%% --- Style Definitions ---
    classDef external fill:#424242,color:white
    classDef pbx fill:#d32f2f,color:white
    classDef gateway fill:#0097a7,stroke:#006064,stroke-width:4px,color:white
    classDef cluster_supervisor fill:#006064,color:white
    classDef worker_process fill:#b2ebf2,stroke:#0097a7,stroke-width:2px,color:#004d40
    classDef mcp_server fill:#fff,stroke:#0097a7,stroke-width:1px,color:#004d40
    classDef ai_model fill:#fce4ec,stroke:#c2185b,stroke-width:2px,color:#880e4f
    classDef human_phone fill:#ef6c00,color:white

%% --- Call Origination ---
    subgraph "External & Home Network"
        A("fa:fa-phone-alt External Caller");
        B["fa:fa-server VoIP Gateway / PBX<br/>(e.g., Fritz!Box, Asterisk)<br/>-"];
        C("fa:fa-tty User Handset<br/>(IP/DECT Phone)<br/>-");
        A -- Phone Call --> B;
    end

%% --- Voice Gateway (The Orchestrator) ---
    subgraph "Voice Gateway Application (PHP + quicpro_async)"
        direction TB
        SIP_Handler["fa:fa-headset SIP Agent<br/>(Call Control)<br/>-"];
        RTP_Handler["fa:fa-exchange-alt RTP Media Handler<br/>(Manages raw audio streams)<br/>-"];
        Logic_Core{"fa:fa-cogs Conversation Logic Core<br/>(Orchestrates AI agents via MCP)<br/>-"};

        SIP_Handler -- "Call events" --> Logic_Core;
        RTP_Handler -- "Audio In/Out" --> Logic_Core;
    end

%% --- AI Microservice Clusters ---
    subgraph "AI Inference Backend (Decoupled & Scalable)"
        direction LR

        subgraph "STT Cluster"
            STT_Supervisor["fa:fa-server Quicpro\Cluster"];
            STT_Worker["fa:fa-cogs STT Worker Process<br/>-"];
            subgraph "Inside STT Worker"
                STT_MCP["MCP Server"];
                Whisper_Model("fa:fa-microphone-alt Whisper Model");
                STT_MCP -.-> Whisper_Model;
            end
            STT_Supervisor -->|"fork()"| STT_Worker;
            STT_Worker --> STT_MCP;
        end

        subgraph "LLM Cluster"
            LLM_Supervisor["fa:fa-server Quicpro\Cluster"];
            LLM_Worker["fa:fa-cogs LLM Worker Process<br/>-"];
            subgraph "Inside LLM Worker"
                LLM_MCP["MCP Server"];
                LLM_Model("fa:fa-brain Llama 3 Model");
                LLM_MCP -.-> LLM_Model;
            end
            LLM_Supervisor -->|"fork()"| LLM_Worker;
            LLM_Worker --> LLM_MCP;
        end

        subgraph "Pluggable Text-to-Speech (TTS) Subsystem"
            TTS_Router["fa:fa-route TTS Router Agent<br/>(Selects Engine)<br/>-"];

            subgraph "Local TTS Engines (Self-hosted)"
                TTS_Piper["fa:fa-cogs Piper TTS Worker"];
                TTS_Bark["fa:fa-cogs Bark TTS Worker"];
            end

            subgraph "Cloud TTS Engines (API Adapters)"
                TTS_Google["fa:fa-cogs Google Cloud TTS Worker"];
                TTS_OpenAI["fa:fa-cogs OpenAI TTS Worker"];
                TTS_Polly["fa:fa-cogs AWS Polly TTS Worker"];
            end
        end
    end

%% --- External Cloud APIs ---
    subgraph "External Cloud APIs"
        API_Google(fa:fa-google Google Cloud);
        API_OpenAI(fa:fa-robot OpenAI API);
        API_AWS(fa:fa-aws AWS);
    end

%% --- Data & Control Flows ---
    B -- "SIP Invite / RTP Audio Stream" --> SIP_Handler & RTP_Handler;
    Logic_Core -- "MCP Call<br/>SpeechToText.transcribe(audio)<br/>-" --> STT_MCP;
    STT_MCP -- "MCP Response<br/>{text: '...'}<br/>-" --> Logic_Core;

    Logic_Core -- "MCP Call<br/>LLM.decide(text)<br/>-" --> LLM_MCP;
    LLM_MCP -- "MCP Response<br/>{action: 'speak' or 'escalate'}<br/>-" --> Logic_Core;

    Logic_Core -- "IF action == 'speak'<br/>MCP Call: TTS.synthesize(text)<br/>-" --> TTS_Router;
    TTS_Router -- "Routes to" --> TTS_Piper & TTS_Bark & TTS_Google & TTS_OpenAI & TTS_Polly;

    TTS_Google -- "API Call" --> API_Google;
    TTS_OpenAI -- "API Call" --> API_OpenAI;
    TTS_Polly -- "API Call" --> API_AWS;

    TTS_Router -- "MCP Response<br/>{audio: '...'}<br/>-" --> Logic_Core;
    Logic_Core -- "Generated Audio" --> RTP_Handler;
    RTP_Handler -- "RTP Audio Stream<br/>(AI's Voice)<br/>-" --> B;

    Logic_Core -- "IF action == 'escalate'<br/>TR-064 Call: DialNumber()<br/>-" --> B;
    B -- "Rings" --> C;

%% Apply Styles
    class A,C,API_Google,API_OpenAI,API_AWS external;
    class B pbx;
    class SIP_Handler,RTP_Handler,Logic_Core gateway;
    class STT_Supervisor,LLM_Supervisor cluster_supervisor;
    class STT_Worker,LLM_Worker worker_process;
    class STT_MCP,LLM_MCP mcp_server;
    class Whisper_Model,LLM_Model ai_model;
    class C human_phone;
    class TTS_Router,TTS_Piper,TTS_Bark,TTS_Google,TTS_OpenAI,TTS_Polly ai_cluster;
~~~


~~~mermaid
graph TD
    %% --- Style Definitions ---
    classDef external fill:#424242,color:white
    classDef fritzbox fill:#d32f2f,color:white
    classDef gateway fill:#0097a7,stroke:#006064,stroke-width:4px,color:white
    classDef cluster_supervisor fill:#006064,color:white
    classDef worker_process fill:#b2ebf2,stroke:#0097a7,stroke-width:2px,color:#004d40
    classDef mcp_server fill:#fff,stroke:#0097a7,stroke-width:1px,color:#004d40
    classDef ai_model fill:#fce4ec,stroke:#c2185b,stroke-width:2px,color:#880e4f
    classDef human_phone fill:#ef6c00,color:white

    %% --- Call Origination ---
    subgraph "External & Home Network"
        A("fa:fa-phone-alt External Caller");
        B["fa:fa-server Fritz!Box"];
        C("fa:fa-tty Fritz!Fon Handset");
        A -- "Phone Call" --> B;
    end

    %% --- Voice Gateway (The Orchestrator) ---
    subgraph "Voice Gateway Application (PHP + Quicpro)"
        direction TB
        SIP_Handler["fa:fa-headset SIP Agent<br/>(Call Control)<br/>-"];
        RTP_Handler["fa:fa-exchange-alt RTP Media Handler<br/>(Manages raw audio streams)<br/>-"];
        Logic_Core{"fa:fa-cogs Conversation Logic Core<br/>(Orchestrates AI agents via MCP)<br/>-"};
        
        SIP_Handler -- "Call events" --> Logic_Core;
        RTP_Handler -- "Audio In/Out" --> Logic_Core;
    end
    
    %% --- AI Microservice Clusters ---
    subgraph "AI Inference Backend (Decoupled & Scalable)"
        direction LR
        
        subgraph "STT Cluster"
            STT_Supervisor["fa:fa-server Quicpro\Cluster"];
            STT_Worker["fa:fa-cogs STT Worker Process<br/>(stt_agent.php)<br/>-"];
            subgraph "Inside STT Worker"
                STT_MCP["MCP Server"];
                Whisper_Model("fa:fa-microphone-alt Whisper Model<br/>Instance<br/>-");
                STT_MCP -.-> Whisper_Model;
            end
            STT_Supervisor -->|"fork()"| STT_Worker;
            STT_Worker --> STT_MCP;
        end

        subgraph "LLM Cluster"
            LLM_Supervisor["fa:fa-server Quicpro\Cluster"];
            LLM_Worker["fa:fa-cogs LLM Worker Process<br/>(llm_agent.php)<br/>-"];
             subgraph "Inside LLM Worker"
                LLM_MCP["MCP Server"];
                LLM_Model("fa:fa-brain Llama 3 Model<br/>Instance<br/>-");
                LLM_MCP -.-> LLM_Model;
            end
            LLM_Supervisor -->|"fork()"| LLM_Worker;
            LLM_Worker --> LLM_MCP;
        end

        subgraph "TTS Cluster"
            TTS_Supervisor["fa:fa-server Quicpro\Cluster"];
            TTS_Worker["fa:fa-cogs TTS Worker Process<br/>(tts_agent.php)<br/>-"];
            subgraph "Inside TTS Worker"
                TTS_MCP["MCP Server"];
                Piper_Model("fa:fa-comment-dots Piper TTS Model<br/>Instance<br/>-");
                TTS_MCP -.-> Piper_Model;
            end
            TTS_Supervisor -->|"fork()"| TTS_Worker;
            TTS_Worker --> TTS_MCP;
        end
    end

    %% --- Data & Control Flows ---
    B -- "SIP Invite" --> SIP_Handler;
    SIP_Handler -- "SIP 200 OK" --> B;
    B -- "RTP Audio Stream<br/>(Caller's Voice)<br/>-" --> RTP_Handler;
    
    Logic_Core -- "MCP Call<br/>SpeechToText.transcribe(audio)<br/>-" --> STT_MCP;
    STT_MCP -- "MCP Response<br/>{text: '...'}<br/>-" --> Logic_Core;
    
    Logic_Core -- "MCP Call<br/>LLM.decide(text)<br/>-" --> LLM_MCP;
    LLM_MCP -- "MCP Response<br/>{action: 'speak' or 'escalate'}<br/>-" --> Logic_Core;

    Logic_Core -- "IF action == 'speak'<br/>MCP Call: TTS.synthesize(text)<br/>-" --> TTS_MCP;
    TTS_MCP -- "MCP Response<br/>{audio: '...'}<br/>-" --> Logic_Core;
    Logic_Core -- "Generated Audio" --> RTP_Handler;
    RTP_Handler -- "RTP Audio Stream<br/>(AI's Voice)<br/>-" --> B;
    
    Logic_Core -- "IF action == 'escalate'<br/>TR-064 Call: DialNumber()<br/>-" --> B;
    B -- "Rings" --> C;
    
    %% Apply Styles
    class A,C external;
    class B fritzbox;
    class SIP_Handler,RTP_Handler,Logic_Core gateway;
    class STT_Supervisor,LLM_Supervisor,TTS_Supervisor cluster_supervisor;
    class STT_Worker,LLM_Worker,TTS_Worker worker_process;
    class STT_MCP,LLM_MCP,TTS_MCP mcp_server;
    class Whisper_Model,LLM_Model,Piper_Model ai_model;
    class C human_phone;
~~~

~~~mermaid

graph TD
%% --- Style Definitions ---
classDef external fill:#424242,color:white
classDef fritzbox fill:#d32f2f,color:white
classDef quicpro_app fill:#0097a7,stroke:#006064,stroke-width:4px,color:white
classDef processing_pipe fill:#b2ebf2,stroke:#0097a7,stroke-width:2px,color:#004d40
classDef ai_models fill:#f8bbd0,stroke:#c2185b,stroke-width:2px,color:#880e4f
classDef internal_device fill:#ef6c00,color:white

    subgraph "External & Home Network"
        A(fa:fa-phone-alt External Caller) -- PSTN/VoIP --> B[fa:fa-server Fritz!Box];
        C(fa:fa-tty Fritz!Fon Handset) <--> B;
    end

    subgraph "Our Application (quicpro_async)<br/>-"
        
        subgraph "AI Answering Machine (PHP Server)<br/>-"
            D["fa:fa-headset SIP Agent<br/>(Call Control)<br/>-"];
            E["fa:fa-exchange-alt RTP Media Handler<br/>(Audio Stream)<br/>-"];
            F{"fa:fa-cogs LLM Core<br/>(Decision Engine)<br/>-"};
        end
        
        subgraph "AI Processing Pipeline (Local Models)<br/>-"
            G["fa:fa-microphone-alt Speech-to-Text<br/>(Whisper)<br/>-"];
            H["fa:fa-comment-dots Text-to-Speech<br/>(TTS)<br/>-"];
        end
        
        B -- "SIP Invite" --> D;
        D -- "SIP 200 OK" --> B;
        B -- "RTP Audio Stream" --> E;
        
        E -- "Raw Audio" --> G;
        G -- "Transcribed Text" --> F;
        F -- "Text for Response" --> H;
        H -- "Generated Audio" --> E;
        E -- "RTP Audio Stream" --> B;
        
        subgraph "Decision Path"
            F -- "Is call important?" --> I{Decision};
            I -- "No, handle directly" --> H;
            I -- "Yes, forward to human" --> J["fa:fa-share-square TR-064 Command<br/>(Dial Handset)<br/>-"];
        end

        J -- "Control Call" --> B;
        
    end

    class A,C internal_device;
    class B fritzbox;
    class D,E,F quicpro_app;
    class G,H,I,J processing_pipe;
~~~