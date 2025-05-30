<?php
/*
 * Application Bootstrap / Configuration
 * This runs once to set up the PipelineOrchestrator and its tool handlers.
 */

// --- Define Core Proto Schemas (as needed by bridges/agents) ---
// For GraphEventLoggerAgent (simplified example)
Quicpro\Proto::defineSchema('WorkflowEventLog', [
    'event_type' => ['type' => 'string', 'tag' => 1], // e.g., "step_completed", "workflow_finished"
    'workflow_id' => ['type' => 'string', 'tag' => 2, 'optional' => true],
    'step_id'    => ['type' => 'string', 'tag' => 3, 'optional' => true],
    'input_hash' => ['type' => 'string', 'tag' => 4, 'optional' => true], // Hash of input data
    'output_hash'=> ['type' => 'string', 'tag' => 5, 'optional' => true], // Hash of output data
    'data_payload_json' => ['type' => 'string', 'tag' => 6, 'optional' => true], // Actual input/output data if small
    'timestamp_ns' => ['type' => 'int64', 'tag' => 7]
]);

// For GraphRAGAgent Request
Quicpro\Proto::defineSchema('GraphRAGContextRequest', [
    'topics'        => ['type' => 'repeated_string', 'tag' => 1], // Keywords or topics
    'context_depth' => ['type' => 'int32', 'tag' => 2, 'default' => 2], // Abstract "hops"
    'max_tokens'    => ['type' => 'int32', 'tag' => 3, 'default' => 500] // Max context tokens
]);
// For GraphRAGAgent Response
Quicpro\Proto::defineSchema('GraphRAGContextResponse', [
    'retrieved_context_text' => ['type' => 'string', 'tag' => 1],
    'source_nodes_count'     => ['type' => 'int32', 'tag' => 2, 'optional' => true]
]);

// For the LLM Agent (e.g., an Ollama Bridge)
Quicpro\Proto::defineSchema('LLMBridgeRequest', [
    'model'             => ['type' => 'string', 'tag' => 1],
    'system_prompt'     => ['type' => 'string', 'tag' => 2, 'optional' => true],
    'user_prompt'       => ['type' => 'string', 'tag' => 3],
    'context_text'      => ['type' => 'string', 'tag' => 4, 'optional' => true], // For RAG
    'options'           => ['type' => 'map_string_string', 'tag' => 5, 'optional' => true]
]);
Quicpro\Proto::defineSchema('LLMBridgeResponse', [
    'generated_text'    => ['type' => 'string', 'tag' => 1],
    'error_message'     => ['type' => 'string', 'tag' => 2, 'optional' => true],
]);


/*
 * Configure the PipelineOrchestrator for automated context logging.
 * The C-native orchestrator will send MCP messages (using 'WorkflowEventLog' schema)
 * to the specified 'GraphEventLoggerAgent'.
 */
Quicpro\PipelineOrchestrator::enableAutoContextLogging([
    'mcp_target' => [
        'host'           => 'graph-event-logger.mcp.local',
        'port'           => 7020,
        'service_name'   => 'ContextLoggerService',
        'method_name'    => 'logEvent',
        'proto_schema'   => 'WorkflowEventLog' // Schema used for logging events
    ],
    'batch_interval_ms' => 5000, // How often to send batched events
    'batch_size'        => 100   // Max events per batch
]);


/*
 * Configure the 'GenerateText' tool handler.
 * This handler now knows how to optionally use a GraphRAGAgent.
 */
Quicpro\PipelineOrchestrator::registerToolHandler(
    'GenerateText',
    [
        'mcp_target' => [ /* Details for the LLM Bridge Agent, e.g., Ollama Bridge */
            'host'           => 'ollama-mcp-bridge.local',
            'port'           => 8000,
            'service_name'   => 'OllamaService',
            'method_name'    => 'chatWithContext' // Expects context_text in its request
        ],
        'input_proto_schema'  => 'LLMBridgeRequest',
        'output_proto_schema' => 'LLMBridgeResponse',
        'param_map' => [ // Maps generic tool params to the LLM bridge's schema
            'system_prompt'      => 'system_prompt',
            'user_prompt'        => 'user_prompt', // This will be the primary input
            'model_identifier'   => 'model',
            'llm_options'        => 'options',
            /* 'context_topic' and 'context_depth' are special params for RAG */
        ],
        'output_map' => ['generated_text' => 'generated_text', 'error' => 'error_message'],

        /* Configuration for the integrated GraphRAG step */
        'rag_config' => [
            'enabled_param' => 'use_graph_context', // If pipeline step params include `use_graph_context: true`
            'mcp_target'    => [ // The GraphRAGAgent that provides context
                'host'         => 'graph-rag-provider.mcp.local',
                'port'         => 7021,
                'service_name' => 'GraphRAGService',
                'method_name'  => 'getContextForTopics'
            ],
            'request_proto_schema' => 'GraphRAGContextRequest',
            'response_proto_schema' => 'GraphRAGContextResponse',
            'context_output_field' => 'retrieved_context_text', // Field from GraphRAGResponse
            // How to get topics for GraphRAGContextRequest:
            // 1. From a specific parameter in the GenerateText step:
            'topics_from_param'    => 'context_topics_list', // e.g. params.context_topics_list = ["QUIC", "networking"]
            // 2. OR, derive topics from a previous step's output (e.g., keywords from ExtractKeywords tool)
            'topics_from_previous_step_output' => [
                'tool_name' => 'ExtractKeywords', // If an ExtractKeywords step ran before GenerateText
                'field_name' => 'keywords'         // Use the 'keywords' array from its output
            ],
            // How to map RAG request fields:
            'rag_param_map' => [
                // 'topics' is handled by 'topics_from_param' or 'topics_from_previous_step_output'
                'context_depth'      => 'context_depth', // e.g. params.context_depth = 3
                'max_context_tokens' => 'max_tokens'   // e.g. params.max_context_tokens = 1000
            ]
        ]
    ]
);
