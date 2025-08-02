<?php
/**
 * Advanced Example: An Interactive Coding Agent with a Clarification Loop
 *
 * This script demonstrates how the Quicpro\PipelineOrchestrator can be used to create
 * a "thoughtful" AI agent that avoids making assumptions. If a user's request is
 * ambiguous, the pipeline is designed to pause, ask the user for clarification
 * via a WebSocket channel, and then resume with the new information.
 *
 * This showcases a stateful, interactive workflow built on top of the stateless
 * C-native orchestrator.
 */

// Assume the Quicpro PECL extension and its classes are available.
use Quicpro\PipelineOrchestrator;
use Quicpro\IIBIN; // Use the final class name directly.

// --- 1. Bootstrap Phase: Define Schemas and Register Tool Handlers ---
// This part would run once when your application starts.

/**
 * Defines all necessary IIBIN schemas for communication between tools.
 */
function register_coding_agent_schemas(): void
{
    // For analyzing the initial request
    IIBIN::defineSchema('RequirementAnalysisRequest', [
        'user_request' => ['type' => 'string', 'tag' => 1]
    ]);
    IIBIN::defineSchema('RequirementAnalysisResponse', [
        'task_summary'            => ['type' => 'string', 'tag' => 1],
        'is_ambiguous'            => ['type' => 'bool',   'tag' => 2],
        'clarification_question'  => ['type' => 'string', 'tag' => 3, 'optional' => true],
    ]);

    // For sending a clarification back to the user
    IIBIN::defineSchema('SendClarificationRequest', [
        'session_id' => ['type' => 'string', 'tag' => 1],
        'question'   => ['type' => 'string', 'tag' => 2]
    ]);
    IIBIN::defineSchema('SendClarificationResponse', [
        'message_sent' => ['type' => 'bool', 'tag' => 1]
    ]);

    // For the final code generation
    IIBIN::defineSchema('CodeGenerationRequest', [
        'unambiguous_task' => ['type' => 'string', 'tag' => 1],
        'user_clarification' => ['type' => 'string', 'tag' => 2, 'optional' => true]
    ]);
    IIBIN::defineSchema('CodeGenerationResponse', [
        'generated_code' => ['type' => 'string', 'tag' => 1],
        'language'       => ['type' => 'string', 'tag' => 2]
    ]);
}

/**
 * Registers the handlers for all MCP agents (tools) used in the pipeline.
 */
function register_coding_agent_tools(): void
{
    // A tool that analyzes the user's request for ambiguity.
    Quicpro\PipelineOrchestrator::registerToolHandler('AnalyzeRequirements', [
        'mcp_target' => ['host' => 'analysis-agent.mcp.local', 'port' => 7100, 'service_name' => 'AnalysisService', 'method_name' => 'analyze'],
        'input_proto_schema'  => 'RequirementAnalysisRequest',
        'output_proto_schema' => 'RequirementAnalysisResponse',
    ]);

    // A special tool that connects to a WebSocket server to send a message to the user.
    Quicpro\PipelineOrchestrator::registerToolHandler('AskUserForClarification', [
        'mcp_target' => ['host' => 'websocket-bridge.mcp.local', 'port' => 7101, 'service_name' => 'WebSocketBridge', 'method_name' => 'sendMessage'],
        'input_proto_schema'  => 'SendClarificationRequest',
        'output_proto_schema' => 'SendClarificationResponse',
    ]);

    // The tool that generates the final code once all ambiguities are resolved.
    Quicpro\PipelineOrchestrator::registerToolHandler('GenerateCode', [
        'mcp_target' => ['host' => 'codegen-agent.mcp.local', 'port' => 7102, 'service_name' => 'CodeGenService', 'method_name' => 'generate'],
        'input_proto_schema'  => 'CodeGenerationRequest',
        'output_proto_schema' => 'CodeGenerationResponse',
    ]);
}

// Run the bootstrap functions.
register_coding_agent_schemas();
register_coding_agent_tools();

// --- 2. Main Application Logic ---

/**
 * This function simulates the main application loop that processes user requests.
 * It shows how a pipeline can be run, paused, and resumed.
 */
function process_user_request(string $userRequest, ?string $userClarification = null): array
{
    echo "--- Processing Request ---\n";
    echo "User Request: \"{$userRequest}\"\n";
    if ($userClarification) {
        echo "With Clarification: \"{$userClarification}\"\n";
    }

    // The initial data provided to the pipeline.
    $initialData = [
        'main_user_request' => $userRequest,
        'user_clarification_if_any' => $userClarification,
        'user_session_id' => 'session_abc123' // ID to route WebSocket message back to correct user
    ];

    $pipelineDefinition = [
        [
            'id' => 'AnalysisStep', 'tool' => 'AnalyzeRequirements', 'input_map' => ['user_request' => '@initial.main_user_request']
        ],
        [
            'id'   => 'ConditionalCheck', 'tool' => 'ConditionalLogic',
            'input_map' => ['condition_field_path' => '@AnalysisStep.is_ambiguous', 'value_to_compare' => true],
            'params' => ['operator' => 'equals']
        ],
        [
            'id'   => 'AskUserStep', 'tool' => 'AskUserForClarification',
            'condition_true_only' => true, // This step runs ONLY if the request is ambiguous.
            'input_map' => [
                'session_id' => '@initial.user_session_id',
                'question'   => '@AnalysisStep.clarification_question'
            ]
        ],
        [
            'id'   => 'CodeGenerationStep', 'tool' => 'GenerateCode',
            // This step runs ONLY if the request is NOT ambiguous.
            'input_map' => [
                'unambiguous_task'   => '@AnalysisStep.task_summary',
                'user_clarification' => '@initial.user_clarification_if_any'
            ]
        ]
    ];

    // --- Execute the pipeline ---
    echo "\nRunning pipeline...\n";
    $result = Quicpro\PipelineOrchestrator::run($initialData, $pipelineDefinition);

    // --- Analyze the result ---
    if (!$result->isSuccess()) {
        echo "\nPipeline FAILED: " . $result->getErrorMessage() . "\n";
        return ['status' => 'error', 'data' => $result->getErrorMessage()];
    }

    // Check the output of the analysis step to see if we need to pause.
    $analysisOutput = $result->getOutputOfStep('AnalysisStep');
    if ($analysisOutput && ($analysisOutput['is_ambiguous'] ?? false) && $userClarification === null) {
        echo "\nACTION REQUIRED: Pipeline paused, awaiting user input.\n";
        echo "Clarification Question Sent to User: \"" . ($analysisOutput['clarification_question'] ?? '') . "\"\n";
        return [
            'status'   => 'awaiting_clarification',
            'question' => $analysisOutput['clarification_question'],
        ];
    }

    // If we reach here, it means the request was not ambiguous or has been clarified.
    $codeGenOutput = $result->getOutputOfStep('CodeGenerationStep');
    if ($codeGenOutput && !empty($codeGenOutput['generated_code'])) {
        echo "\nSUCCESS: Pipeline completed.\n";
        return [
            'status' => 'completed',
            'data'   => $codeGenOutput,
        ];
    }

    echo "\nPipeline finished with an indeterminate state.\n";
    return ['status' => 'unknown', 'data' => $result->getAllOutputs()];
}

// --- SIMULATION ---

// Round 1: User makes an ambiguous request
$initialRequest = "Create a User class.";
$result1 = process_user_request($initialRequest);

echo "\n======================================================\n";

// Check the result of the first run.
if (($result1['status'] ?? '') === 'awaiting_clarification') {
    echo "Simulating user response via WebSocket...\n";
    $simulatedUserAnswer = "Use public properties, no getters/setters.";

    // Round 2: The application receives the user's answer and re-runs the process,
    // providing the clarification in the initial data.
    $result2 = process_user_request($initialRequest, $simulatedUserAnswer);

    if (($result2['status'] ?? '') === 'completed') {
        echo "\nFinal Generated Code:\n";
        echo "---------------------\n";
        echo $result2['data']['generated_code'] . "\n";
    }
}