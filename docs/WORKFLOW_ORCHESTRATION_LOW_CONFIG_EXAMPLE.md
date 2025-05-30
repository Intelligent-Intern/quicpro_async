This document shows how to configure the `PipelineOrchestrator` to use different LLM backends (OpenAI, Azure OpenAI) for the generic `GenerateText` tool, while keeping the end-user's PHP code for defining and running pipelines extremely simple and unchanged.

## Core Principle: Consistent User API, Configurable Backend

The user interacts with a high-level abstraction like `Quicpro\PipelineOrchestrator::run()` and uses generic tool names like `GenerateText`. The specific backend (Ollama, OpenAI, Azure, etc.) and the necessary MCP communication details are configured separately.

## 1. Configuration for OpenAI Backend

First, you would need an MCP bridge service that translates MCP requests into OpenAI API calls. This bridge would handle authentication (OpenAI API Key) and other specifics.

Then, in your application's bootstrap or configuration phase, you register this bridge as the handler for the `GenerateText` tool:

~~~php
<?php
// In your application's bootstrap file (e.g., config.php, bootstrap.php)

// Define Proto schemas for interacting with your OpenAI MCP Bridge
Quicpro\Proto::defineSchema('OpenAiBridgeRequest', [
    'model'    => ['type' => 'string', 'tag' => 1], // e.g., "gpt-4-turbo", "gpt-3.5-turbo"
    'prompt'   => ['type' => 'string', 'tag' => 2, 'optional' => true],
    'messages' => ['type' => 'repeated_map_string_string', 'tag' => 3, 'optional' => true],
    'options'  => ['type' => 'map_string_string', 'tag' => 4, 'optional' => true] // For temperature, max_tokens etc.
]);
Quicpro\Proto::defineSchema('OpenAiBridgeResponse', [
    'generated_text' => ['type' => 'string', 'tag' => 1],
    'error_message'  => ['type' => 'string', 'tag' => 2, 'optional' => true],
]);

Quicpro\PipelineOrchestrator::registerToolHandler(
    'GenerateText', // Generic tool name
    [
        'mcp_target' => [
            'host' => 'openai-mcp-bridge.yourdomain.com', // Your OpenAI MCP bridge host
            'port' => 8001,                               // Your OpenAI MCP bridge port
            'service_name' => 'OpenAiChatService',        // Service name on your bridge
            'method_name'  => 'generateCompletion'        // Method name on your bridge
        ],
        'input_proto_schema' => 'OpenAiBridgeRequest',
        'output_proto_schema' => 'OpenAiBridgeResponse',
        'param_map' => [ // Maps generic tool params to the bridge's schema fields
            'model_identifier'   => 'model',
            // 'pipeline_input_prompt' is handled by orchestrator if initialData is string
            // 'pipeline_input_messages' is handled by orchestrator if initialData is array of messages
            'llm_options'        => 'options'
        ],
        'output_map' => [ // Maps bridge's response back to generic tool output
            'generated_text'     => 'generated_text', // Field names match here
            'error'              => 'error_message'
        ]
    ]
);
?>
~~~

## 2. Configuration for Azure OpenAI Backend

Similarly, for Azure OpenAI, you'd have a dedicated MCP bridge. Azure requires specific endpoint details (resource name, deployment ID) and an API key. The bridge would manage these.

Configuration in your application's bootstrap:

~~~php
<?php
// In your application's bootstrap file (e.g., config.php, bootstrap.php)

// Define Proto schemas for interacting with your Azure OpenAI MCP Bridge
// These might be very similar or identical to the OpenAI ones if the bridge abstracts it well.
Quicpro\Proto::defineSchema('AzureOpenAiBridgeRequest', [
    'deployment_or_model' => ['type' => 'string', 'tag' => 1], // Could be deployment ID or a model name the bridge maps
    'prompt'              => ['type' => 'string', 'tag' => 2, 'optional' => true],
    'messages'            => ['type' => 'repeated_map_string_string', 'tag' => 3, 'optional' => true],
    'options'             => ['type' => 'map_string_string', 'tag' => 4, 'optional' => true]
]);
Quicpro\Proto::defineSchema('AzureOpenAiBridgeResponse', [
    'generated_text' => ['type' => 'string', 'tag' => 1],
    'error_message'  => ['type' => 'string', 'tag' => 2, 'optional' => true],
]);

Quicpro\PipelineOrchestrator::registerToolHandler(
    'GenerateText', // Generic tool name
    [
        'mcp_target' => [
            'host' => 'azure-openai-mcp-bridge.yourdomain.com', // Your Azure OpenAI MCP bridge host
            'port' => 8002,                                    // Your Azure OpenAI MCP bridge port
            'service_name' => 'AzureAiChatService',             // Service name on your bridge
            'method_name'  => 'generateAzureCompletion'         // Method name on your bridge
        ],
        'input_proto_schema' => 'AzureOpenAiBridgeRequest',
        'output_proto_schema' => 'AzureOpenAiBridgeResponse',
        'param_map' => [
            'model_identifier'   => 'deployment_or_model', // Generic 'model_identifier' maps to 'deployment_or_model'
            'llm_options'        => 'options'
        ],
        'output_map' => [
            'generated_text'     => 'generated_text',
            'error'              => 'error_message'
        ]
    ]
);
?>
~~~

## 3. User's PHP Code (Remains Unchanged and Simple)

Regardless of whether `GenerateText` is configured to use the Ollama, OpenAI, or Azure bridge, the "noob" user's code for a simple LLM call remains the same:

**Scenario A: Simple String Prompt**

~~~php
<?php
// Assumes one of the above configurations (Ollama, OpenAI, or Azure) for 'GenerateText'
// has been executed in the application's bootstrap.
// Assumes php.ini might have: quicpro.llm.default_model = "default-model-for-the-configured-bridge"

$userPrompt = "What are the key features of QUIC protocol?";

$pipelineDefinition = [
    // The orchestrator uses $userPrompt as input for 'GenerateText'.
    // If 'model_identifier' is omitted in params, the default from php.ini is used,
    // which should be compatible with the configured bridge (Ollama, OpenAI, or Azure).
    ['tool' => 'GenerateText', 'params' => ['llm_options' => ['temperature' => 0.7]]]
    // To target a specific model for the configured bridge:
    // ['tool' => 'GenerateText', 'params' => ['model_identifier' => 'gpt-4o', 'llm_options' => ['temperature' => 0.7]]]
];

$result = Quicpro\PipelineOrchestrator::run($userPrompt, $pipelineDefinition);

if ($result->isSuccess()) {
    echo "LLM Response:\n" . ($result->getFinalOutput()['generated_text'] ?? 'No response received.') . "\n";
} else {
    echo "Pipeline Error: " . $result->getErrorMessage() . "\n";
}
?>
~~~

**Scenario B: Structured Chat Messages Input**

~~~php
<?php
// Assumes one of the above configurations for 'GenerateText' is active.

$chatMessages = [
    ['role' => 'system', 'content' => 'You are an expert on network protocols.'],
    ['role' => 'user', 'content' => 'Compare QUIC and TCP in terms of connection establishment.']
];

$pipelineDefinition = [
    // The orchestrator uses $chatMessages as input for the 'GenerateText' tool.
    ['tool' => 'GenerateText'] // Model from php.ini, default options
];

$result = Quicpro\PipelineOrchestrator::run($chatMessages, $pipelineDefinition);

if ($result->isSuccess()) {
    echo "LLM Chat Response:\n" . ($result->getFinalOutput()['generated_text'] ?? 'No response received.') . "\n";
} else {
    echo "Pipeline Error: " . $result->getErrorMessage() . "\n";
}
?>
~~~

By using `PipelineOrchestrator::registerToolHandler`, you abstract the backend specifics. The user of the pipeline only needs to know the generic tool name (`GenerateText`) and its expected generic parameters. The orchestrator, based on the registration, routes the call to the appropriate MCP bridge (Ollama, OpenAI, Azure, etc.) and handles the necessary parameter and result mapping.