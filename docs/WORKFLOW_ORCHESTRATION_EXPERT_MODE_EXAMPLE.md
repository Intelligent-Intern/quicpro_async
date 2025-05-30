# Expert Guide: High-Performance Pipelines with Quicpro for Demanding Scenarios

This guide is targeted at developers and architects aiming to build ultra-low-latency, high-throughput workflows using the `Quicpro` PHP extension. We will focus on leveraging its C-native `PipelineOrchestrator` engine for scenarios like High-Frequency Trading (HFT) or complex AI agent interactions, emphasizing advanced configuration for maximum performance.

The `Quicpro` extension provides a powerful foundation with its core `MCP` (Model Context Protocol) over QUIC and efficient `Proto` serialization. The `PipelineOrchestrator`, primarily implemented in C for optimal performance, executes defined sequences of operations (pipelines). The PHP `Quicpro\PipelineOrchestrator` class serves as a user-friendly API to this native engine. A key design principle is the decoupling of specific backend technologies, such as graph databases for context management, from the core C extension. This allows for flexibility, enabling integration through PHP user-land code or specialized MCP agents.

## Core Concepts

* **`Quicpro\PipelineOrchestrator`**: A high-performance pipeline execution engine with its core logic implemented in C within the PECL extension. It manages data flow between steps, invokes the necessary MCP agents via QUIC, handles `Proto` serialization/deserialization, and aggregates errors. A PHP class acts as the API wrapper.
* **Tool Handlers**: The `PipelineOrchestrator` relies on "tool handlers" configured via the static PHP method `Quicpro\PipelineOrchestrator::registerToolHandler()`. This crucial configuration step maps generic tool names used in pipeline definitions (e.g., `GenerateText`, or custom names like `FetchMarketDataHFT`) to specific MCP agent endpoints. It also defines their communication parameters, including highly tuned `Quicpro\MCP` client options that are utilized by the C-native MCP communication layer.
* **`Quicpro\Proto` Schemas**: Data contracts for requests and responses between pipeline steps and MCP agents are defined using `Quicpro\Proto`. This C-native serialization mechanism ensures efficient and validated data exchange.

## Standard Predefined MCP Tools/Workflow Steps

The `Quicpro\PipelineOrchestrator` can be pre-configured or extended by the application to support a variety of common processing tasks as standardized "tools." These tools abstract underlying MCP agent calls, for which handlers are registered. Here is a representative list:

1.  **`FetchUrlContent`**: Fetches content from a URL.
    * Input: `{ "url": "string" }`
    * Output: `{ "http_status": int, "content_type": "string", "body": "bytes|string" }`
2.  **`ExtractTextFromHtml`**: Extracts plain text from HTML content.
    * Input: `{ "html_content": "string" }`
    * Output: `{ "extracted_text": "string" }`
3.  **`SummarizeText`**: Creates a summary of a text.
    * Input: `{ "text_to_summarize": "string", "max_length_words": "int_optional", "style": "string_optional_bullet_points|paragraph" }`
    * Output: `{ "summary": "string" }`
4.  **`TranslateText`**: Translates text to another language.
    * Input: `{ "text": "string", "target_language_code": "string", "source_language_code": "string_optional" }`
    * Output: `{ "translated_text": "string", "detected_source_language": "string" }`
5.  **`DetectLanguage`**: Detects the language of a text.
    * Input: `{ "text_sample": "string" }`
    * Output: `{ "language_code": "string", "confidence": "float" }`
6.  **`ExtractKeywords`**: Extracts keywords from a text.
    * Input: `{ "text_content": "string", "max_keywords": "int_optional" }`
    * Output: `{ "keywords": ["string"] }`
7.  **`SentimentAnalysis`**: Analyzes the sentiment of a text.
    * Input: `{ "text_to_analyze": "string" }`
    * Output: `{ "sentiment_label": "string_positive|negative|neutral", "sentiment_score": "float" }`
8.  **`NamedEntityRecognition` (NER)**: Recognizes named entities (people, places, organizations).
    * Input: `{ "text": "string" }`
    * Output: `{ "entities": [{"text": "string", "type": "string", "offset": "int"}] }`
9.  **`TextClassification`**: Classifies text into predefined categories.
    * Input: `{ "text": "string", "categories": ["string_optional_predefined_model"] }`
    * Output: `{ "classification": "string", "scores": {"category": "float"} }`
10. **`QuestionAnswering`**: Answers a question based on a context text.
    * Input: `{ "context_text": "string", "question": "string" }`
    * Output: `{ "answer": "string", "confidence": "float" }`
11. **`GenerateText` (LLM Call)**: Generates text based on a prompt or messages.
    * Input: `{ "prompt": "string_optional", "messages": "[array_of_maps_optional]", "model_identifier": "string_optional", "max_tokens": "int_optional", "temperature": "float_optional", "llm_options": "map_string_string_optional" }`
    * Output: `{ "generated_text": "string" }`
12. **`ImageClassification`**: Classifies the content of an image.
    * Input: `{ "image_bytes": "bytes" | "image_url": "string" }`
    * Output: `{ "labels": [{"name": "string", "score": "float"}] }`
13. **`ObjectDetectionOnImage`**: Detects objects in an image.
    * Input: `{ "image_bytes": "bytes" | "image_url": "string" }`
    * Output: `{ "objects": [{"label": "string", "score": "float", "bounding_box": [x,y,w,h]}] }`
14. **`OcrImage`**: Extracts text from an image (Optical Character Recognition).
    * Input: `{ "image_bytes": "bytes" | "image_url": "string", "language_hint": "string_optional" }`
    * Output: `{ "extracted_text": "string" }`
15. **`DataValidation`**: Validates data against a given schema (e.g., a registered Proto schema name).
    * Input: `{ "data_to_validate": "any_php_array_or_object", "schema_identifier": "string_schema_name" }`
    * Output: `{ "is_valid": "bool", "errors": ["string_optional"] }`
16. **`DataTransformation` (Simple)**: Performs simple data transformations based on rules.
    * Input: `{ "data_input": "array", "transformation_rules": "array_or_string_dsl" }`
    * Output: `{ "data_output": "array" }`
17. **`CalculateEmbedding`**: Generates a vector embedding for text or data.
    * Input: `{ "content_to_embed": "string|bytes", "embedding_model_hint": "string_optional" }`
    * Output: `{ "embedding_vector": ["float"] }`
18. **`StoreDataRecord`**: Stores a data record in a predefined target.
    * Input: `{ "record_data": "array|object", "storage_target_id": "string", "collection_name": "string_optional" }`
    * Output: `{ "record_id": "string", "status": "string_success|failure" }`
19. **`FetchDataRecord`**: Fetches a data record from a predefined target.
    * Input: `{ "record_id_to_fetch": "string", "storage_target_id": "string", "collection_name": "string_optional" }`
    * Output: `{ "record_data": "array|object", "status": "string_found|not_found" }`
20. **`ConditionalLogic`**: (Meta-step for pipeline flow control) Executes subsequent steps based on a condition.
    * Input: `{ "condition_field_path": "string_from_previous_output", "operator": "string_equals|contains|gt|lt|etc", "value_to_compare": "any" }`
    * Output: `{ "condition_met": "bool" }`

## Context Building and GraphRAG Philosophy

A powerful application of the `PipelineOrchestrator` is the dynamic building of context for AI agents and LLMs. The `Quicpro` C extension itself remains database-agnostic.

1.  **Rich Execution Data from Orchestrator**: The C-native `PipelineOrchestrator` can be designed to produce detailed, structured information about each pipeline execution (e.g., within the `WorkflowResult` object returned by `run()`, or via hooks/callbacks exposed to PHP). This data includes executed steps, inputs, outputs, timings, errors, and contextual links.

2.  **User-Land Driven Context Persistence**: PHP user-land code consumes this rich data. Using standard PHP database drivers (e.g., for Neo4j, ArangoDB, or other graph databases), the application can then transform and persist this information into a graph structure. This graph represents the evolving context of tasks, solutions, agents, and their relationships. This approach offers maximum flexibility regarding database choice and graph schema design.

3.  **"Automode" via MCP GraphEventLoggerAgent**: For a more automated (yet still decoupled) approach, the C-native orchestrator can be configured to emit standardized lifecycle and data events as MCP messages to a designated `GraphEventLoggerAgent`. This specialized agent (which can be a PHP application using `Quicpro\MCP\Server` and a graph database driver) then translates these events into graph database writes. The C extension only needs to know how to send MCP messages, not the specifics of any graph database.

4.  **GraphRAG (Retrieval Augmented Generation)**: Once context is stored in a graph database, it can be used to enhance LLM interactions (GraphRAG). Before a `GenerateText` tool call (or similar AI task), a preliminary step in the pipeline (or logic within the tool handler) can query the graph database. This query, driven by the current task's context, retrieves relevant information (e.g., historical data, related concepts, user preferences). This retrieved information is then used to augment the prompt sent to the LLM, leading to more informed and contextually relevant responses. This retrieval logic typically resides in PHP user-land or a dedicated RAG MCP agent.

This philosophy ensures the core `Quicpro` C extension remains lean, performant, and vendor-neutral, while providing the necessary hooks and data for sophisticated context management and AI augmentation strategies in PHP user-land or through specialized MCP agents.

## HFT Use Case Example

The following sections detail the setup for an HFT-like pipeline, showcasing advanced configuration and usage.

### 1. HFT-Specific Proto Schemas

For HFT, data schemas are typically lean and use efficient data types to minimize serialization overhead and transmission time.

~~~php
<?php
/*
 * Schemas for HFT operations.
 * These define the data contracts for interacting with specialized HFT MCP agents.
 */
Quicpro\Proto::defineSchema('HFTMarketDataRequest', [
    'instrument_id' => ['type' => 'string', 'tag' => 1],
    'exchange_hint' => ['type' => 'string', 'tag' => 2, 'optional' => true]
]);
Quicpro\Proto::defineSchema('HFTMarketDataResponse', [
    'instrument_id' => ['type' => 'string', 'tag' => 1],
    'bid_price'     => ['type' => 'double', 'tag' => 2], /* Consider fixed-point decimals for precision */
    'ask_price'     => ['type' => 'double', 'tag' => 3],
    'timestamp_ns'  => ['type' => 'int64',  'tag' => 4]  /* Nanosecond precision timestamp */
]);

Quicpro\Proto::defineSchema('HFTOrderRequest', [
    'order_id'      => ['type' => 'string', 'tag' => 1],
    'instrument_id' => ['type' => 'string', 'tag' => 2],
    'side'          => ['type' => 'string', 'tag' => 3], /* e.g., "BUY" or "SELL" */
    'price'         => ['type' => 'double', 'tag' => 4],
    'quantity'      => ['type' => 'int32',  'tag' => 5],
    'order_type'    => ['type' => 'string', 'tag' => 6, 'default' => 'LIMIT'] /* e.g., "LIMIT", "MARKET" */
]);
Quicpro\Proto::defineSchema('HFTOrderConfirmation', [
    'order_id'      => ['type' => 'string', 'tag' => 1],
    'status'        => ['type' => 'string', 'tag' => 2], /* e.g., "ACCEPTED", "FILLED", "REJECTED" */
    'fill_price'    => ['type' => 'double', 'tag' => 3, 'optional' => true],
    'fill_quantity' => ['type' => 'int32',  'tag' => 4, 'optional' => true],
    'message'       => ['type' => 'string', 'tag' => 5, 'optional' => true]
]);
?>
~~~

### 2. Expert Configuration of Tool Handlers for HFT Agents

This crucial step, typically performed during application bootstrap, configures the `PipelineOrchestrator` to route specific tool names to your high-performance HFT MCP agents. It allows specifying granular `Quicpro\MCP` client options for each tool handler, which are then utilized by the C-native MCP communication layer.

~~~php
<?php
/*
 * Advanced configuration for HFT MCP agent interactions.
 * This section defines how generic tool names in a pipeline map to
 * specific, highly-tuned MCP agent endpoints.
 */

// Base MCP client options optimized for HFT scenarios
$hftMcpBaseOptions = [
    'tls_enable'            => false, /* Assumes a secure, trusted, ultra-low-latency internal network */
    'session_mode'          => Quicpro\MCP::SESSION_LOW_LATENCY_UNRELIABLE, /* Example: for QUIC datagrams if applicable */
                                     /* Or a highly tuned Quicpro\MCP::SESSION_RELIABLE_STREAM */
    'zero_copy_send'        => true,  /* Attempt to use zero-copy mechanisms for sending data */
    'zero_copy_receive'     => true,  /* Attempt to use zero-copy mechanisms for receiving data */
    'mmap_path_prefix'      => '/dev/shm/quicpro_hft_', /* Prefix for memory-mapped files if zero-copy uses them */
    'cpu_affinity_core_start_index' => 4, /* Suggest starting CPU core for I/O threads of MCP clients */
    'busy_poll_duration_us' => 1000,    /* Duration for busy-polling for network packets to reduce latency */
    'io_uring_enable'       => true,      /* Leverage io_uring on compatible Linux systems for async I/O */
    'disable_congestion_control' => true, /* DANGER: Only for completely controlled, dedicated networks with known capacity */
    'connect_timeout_ms'    => 100,       /* Very short timeout for establishing connections */
    'request_timeout_ms'    => 50,        /* Aggressive timeout for individual MCP requests */
    'max_idle_timeout_ms'   => 1000,      /* Close inactive connections quickly to free resources */
    'log_level'             => Quicpro\MCP::LOG_LEVEL_NONE, /* Disable logging in hot path for maximum performance */
];

// Registering a custom "FetchMarketDataHFT" tool
Quicpro\PipelineOrchestrator::registerToolHandler(
    'FetchMarketDataHFT',
    [
        'mcp_target' => [
            'host' => 'hft-market-data-agent.lan', // Endpoint of the HFT Market Data Agent
            'port' => 9001,
            'service_name' => 'MarketDataFeed',    // MCP Service name on the agent
            'method_name'  => 'getQuote'           // Method to call
        ],
        'mcp_options' => $hftMcpBaseOptions, // Apply the specialized HFT MCP options
        'input_proto_schema' => 'HFTMarketDataRequest',  // Expected input Proto schema
        'output_proto_schema' => 'HFTMarketDataResponse', // Expected output Proto schema
        'param_map' => [ /* Mapping from generic pipeline step parameters to this tool's specific input schema fields */
            'instrument' => 'instrument_id',
            'exchange'   => 'exchange_hint'
        ],
        'output_map' => [ /* Mapping from this tool's output schema fields back to generic pipeline step output fields */
            'instrument' => 'instrument_id',
            'bid'        => 'bid_price',
            'ask'        => 'ask_price',
            'timestamp'  => 'timestamp_ns'
        ]
    ]
);

// Registering a custom "ExecuteOrderHFT" tool
Quicpro\PipelineOrchestrator::registerToolHandler(
    'ExecuteOrderHFT',
    [
        'mcp_target' => [
            'host' => 'hft-order-execution-agent.lan',
            'port' => 9002,
            'service_name' => 'OrderGateway',
            'method_name'  => 'placeOrder'
        ],
        'mcp_options' => $hftMcpBaseOptions,
        'input_proto_schema' => 'HFTOrderRequest',
        'output_proto_schema' => 'HFTOrderConfirmation',
        'param_map' => [
            'order_ref'  => 'order_id',
            'instrument' => 'instrument_id',
            'trade_side' => 'side',
            'limit_price'=> 'price',
            'order_qty'  => 'quantity',
            'type'       => 'order_type'
        ],
        'output_map' => [
            'order_ref'    => 'order_id',
            'exec_status'  => 'status',
            'filled_price' => 'fill_price',
            'filled_qty'   => 'fill_quantity',
            'details'      => 'message'
        ]
    ]
);
?>
~~~

### 3. HFT Pipeline Definition and Execution

With the tool handlers expertly configured, the actual pipeline definition for a trading strategy remains declarative. The C-native `PipelineOrchestrator` executes this logic using the specified high-performance settings.

~~~php
<?php
/*
 * This script defines and executes an HFT-like pipeline.
 * It assumes the 'FetchMarketDataHFT' and 'ExecuteOrderHFT' tool handlers,
 * along with their associated Proto schemas, have been registered.
 */

$instrumentSymbol = 'EURUSD_SPOT';
$clientOrderId = 'HFT_CLIENT_ORD_' . bin2hex(random_bytes(6)); // Unique client order ID

// Initial data for this specific pipeline run
$initialHftData = [
    'target_instrument' => $instrumentSymbol,
    'client_order_id' => $clientOrderId,
    'buy_price_threshold' => 1.08500 // Example: dynamic threshold for placing a buy order
];

// Definition of the HFT trading logic as a pipeline
$hftPipelineDefinition = [
    [
        'tool' => 'FetchMarketDataHFT',
        'input_map' => ['instrument' => '@initial.target_instrument'],
        /* 'params' => ['exchange' => 'PRIMARY_ECN'] // Optional: if direct parameters are needed */
    ],
    [
        'tool' => 'ConditionalLogic', // Using the generic conditional tool
        'input_map' => [
            /* Condition: Is the fetched ask price less than our buy threshold? */
            'condition_field_path' => '@previous.ask',      /* Refers to 'ask' field from FetchMarketDataHFT output */
            'value_to_compare'     => '@initial.buy_price_threshold' /* Refers to 'buy_price_threshold' from $initialHftData */
        ],
        'params' => ['operator' => 'lt'] // 'lt' for "less than"
    ],
    [
        'tool' => 'ExecuteOrderHFT',
        'condition_true_only' => true, /* This step only runs if the preceding ConditionalLogic step's output 'condition_met' was true. */
                                       /* The orchestrator needs to understand this convention or have it explicitly mapped. */
        'input_map' => [
            'instrument' => '@FetchMarketDataHFT.instrument', /* Use 'instrument' from the output of the FetchMarketDataHFT step */
            'trade_side' => 'BUY',                             /* Fixed for this example logic */
            'limit_price'=> '@FetchMarketDataHFT.ask',        /* Use current 'ask' price from market data as the limit */
            'order_qty'  => 100000,                           /* Example fixed quantity */
            'order_ref'  => '@initial.client_order_id'        /* Use 'client_order_id' from initial data */
        ],
        'params' => ['type' => 'LIMIT'] // Specify the order type
    ]
];

// Global pipeline execution options tailored for this HFT scenario
$pipelineExecutionOptions = [
    'overall_timeout_ms' => 200, // An aggressive timeout for the entire pipeline execution
    'fail_fast' => true,         // If any step in the pipeline fails, abort the entire pipeline immediately
];

echo "Initiating HFT Pipeline for instrument: {$instrumentSymbol}...\n";
$result = Quicpro\PipelineOrchestrator::run(
    $initialHftData,
    $hftPipelineDefinition,
    $pipelineExecutionOptions // Pass the expert execution options
);

if ($result->isSuccess()) {
    echo "HFT Pipeline Completed Successfully.\n";
    $marketDataOutput = $result->getOutputOfStep('FetchMarketDataHFT');
    // OrderConfirmationOutput will be null if the ExecuteOrderHFT step was skipped due to the condition
    $orderConfirmationOutput = $result->getOutputOfStep('ExecuteOrderHFT');

    if ($marketDataOutput) {
        echo "Market Data Fetched: Ask Price = " . ($marketDataOutput['ask'] ?? 'N/A') .
             " at " . ($marketDataOutput['timestamp'] ?? 'N/A') . "ns.\n";
    }

    if ($orderConfirmationOutput) {
        echo "Order Execution Result: Status = " . ($orderConfirmationOutput['exec_status'] ?? 'N/A');
        if (isset($orderConfirmationOutput['filled_price'])) {
            echo ", Fill Price = " . $orderConfirmationOutput['filled_price'];
        }
        echo " for Order Ref: " . ($orderConfirmationOutput['order_ref'] ?? 'N/A') . "\n";
    } elseif ($marketDataOutput && ($marketDataOutput['ask'] ?? PHP_INT_MAX) >= $initialHftData['buy_price_threshold']) {
        // This condition implies the order was not placed because the price was not met
        echo "Conditional Order Info: Order not placed as market condition (ask price not below threshold) was not met.\n";
    } else {
        // Generic message if order wasn't placed and it wasn't due to the specific price condition checked above
        echo "Conditional Order Info: Order may not have been placed.\n";
    }

} else {
    echo "HFT Pipeline Failed: " . $result->getErrorMessage() . "\n";
    // Assuming getFailedStepDetails() returns an array like ['step_id' => '...', 'error' => '...']
    $failedStepInfo = $result->getFailedStepDetails();
    if ($failedStepInfo) {
        echo "Failure occurred at step ID '{$failedStepInfo['step_id']}' with error: {$failedStepInfo['error']}\n";
    }
}
?>
~~~

## Key Aspects of Expert Configuration

* **C-Native Orchestration**: The core pipeline execution logic resides in the C extension for maximum throughput and minimal overhead.
* **Granular `Quicpro\MCP` Options**: Through `registerToolHandler`, each MCP agent interaction can be finely tuned for performance (e.g., TLS disablement, zero-copy, aggressive timeouts, CPU affinity hints, congestion control bypass). These options are directly utilized by the C-native MCP client components.
* **Lean Proto Schemas**: Data structures are kept minimal and use efficient types, critical for reducing C-native serialization/deserialization overhead.
* **Aggressive Timeouts**: Both connection and request timeouts, as well as overall pipeline timeouts, are set very low, reflecting the demands of high-performance scenarios.
* **Conditional Logic**: Pipelines can include decision-making steps (`ConditionalLogic`) to react to real-time data, with the flow control managed by the orchestrator.
* **Declarative Pipelines**: Despite the underlying complexity and C-native execution, the definition of the workflow logic remains a relatively simple, declarative structure in PHP. The `PipelineOrchestrator` (via its C engine) abstracts the execution details based on the expert configuration provided during the `registerToolHandler` phase and optionally during the `run` call.

This framework allows for building highly demanding applications by separating the concerns of pipeline logic definition from the expert configuration of the underlying communication and execution mechanisms, all powered by a C-native core.
```