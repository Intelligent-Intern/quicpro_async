<?php
/**
 * Advanced Example: A Pipeline with Integrated CRUD Agents
 *
 * This script demonstrates how the PipelineOrchestrator can leverage specialized
 * CRUD agents to persist and retrieve data as part of a workflow. This allows
 * agents to not only compute results but also to interact with a stateful
 * data store (e.g., a database).
 */

use Quicpro\PipelineOrchestrator;
use Quicpro\IIBIN;

// --- 1. Bootstrap Phase: Define Schemas and Register Tool Handlers ---

/**
 * Defines all necessary IIBIN schemas, including those for CRUD operations.
 */
function register_crud_pipeline_schemas(): void
{
    // Schema for the LLM tool
    IIBIN::defineSchema('TextGenerationRequest', ['prompt' => ['type' => 'string', 'tag' => 1]]);
    IIBIN::defineSchema('TextGenerationResponse', ['generated_text' => ['type' => 'string', 'tag' => 1]]);

    // Schemas for the generic CRUD agent (MCPStorageAgent)
    IIBIN::defineSchema('CreateRecordRequest', [
        'resource_type' => ['type' => 'string', 'tag' => 1], // e.g., "summary", "user_profile"
        'data'          => ['type' => 'map_string_string', 'tag' => 2] // The data to store
    ]);
    IIBIN::defineSchema('CreateRecordResponse', [
        'id'     => ['type' => 'string', 'tag' => 1], // The ID of the newly created record
        'status' => ['type' => 'string', 'tag' => 2]  // e.g., "SUCCESS"
    ]);

    IIBIN::defineSchema('ReadRecordRequest', [
        'resource_type' => ['type' => 'string', 'tag' => 1],
        'id'            => ['type' => 'string', 'tag' => 2]
    ]);
    IIBIN::defineSchema('ReadRecordResponse', [
        'id'     => ['type' => 'string', 'tag' => 1],
        'status' => ['type' => 'string', 'tag' => 2], // e.g., "FOUND", "NOT_FOUND"
        'data'   => ['type' => 'map_string_string', 'tag' => 3, 'optional' => true]
    ]);
}

/**
 * Registers the handlers for the LLM tool and the new CRUD tools.
 * Note how all CRUD tools point to the same StorageAgent but different methods.
 */
function register_crud_pipeline_tools(): void
{
    // The LLM Agent that generates data
    Quicpro\PipelineOrchestrator::registerToolHandler('GenerateText', [
        'mcp_target' => ['host' => 'llm-bridge.mcp.local', 'port' => 8000, 'service_name' => 'LLMService', 'method_name' => 'generate'],
        'input_proto_schema'  => 'TextGenerationRequest',
        'output_proto_schema' => 'TextGenerationResponse',
    ]);

    // --- CRUD Agent Configuration ---
    $storageAgentTarget = [
        'host' => 'storage-agent.mcp.local',
        'port' => 7500,
        'service_name' => 'CRUDService'
        // Method name will be specified per tool
    ];

    // Tool to CREATE a new record
    Quicpro\PipelineOrchestrator::registerToolHandler('CreateRecord', [
        'mcp_target' => array_merge($storageAgentTarget, ['method_name' => 'create']),
        'input_proto_schema'  => 'CreateRecordRequest',
        'output_proto_schema' => 'CreateRecordResponse',
    ]);

    // Tool to READ a record
    Quicpro\PipelineOrchestrator::registerToolHandler('ReadRecord', [
        'mcp_target' => array_merge($storageAgentTarget, ['method_name' => 'read']),
        'input_proto_schema'  => 'ReadRecordRequest',
        'output_proto_schema' => 'ReadRecordResponse',
    ]);

    // Handlers for UpdateRecord and DeleteRecord would be defined similarly...
}

// Run the bootstrap functions for setup.
register_crud_pipeline_schemas();
register_crud_pipeline_tools();


// --- 2. Main Application Logic: Define and Run a Workflow with CRUD ---

$initialData = [
    'original_text' => "QUIC (Quick UDP Internet Connections) is a new transport protocol that reduces latency compared to TCP. It is encrypted by default using TLS 1.3 and handles multiplexing without head-of-line blocking. It was standardized by the IETF and is the foundation of HTTP/3."
];

$pipelineDefinition = [
    [
        'id'   => 'SummarizationStep',
        'tool' => 'GenerateText',
        'input_map' => [
            // Create the prompt by combining static text with initial data.
            'prompt' => "Summarize the following text in one short sentence: '{{@initial.original_text}}'"
        ]
        // Output of this step: { "generated_text": "QUIC is an encrypted, low-latency transport protocol that underlies HTTP/3." }
    ],
    [
        'id'   => 'StoreSummaryStep',
        'tool' => 'CreateRecord',
        'input_map' => [
            // The 'data' field for the CreateRecordRequest is constructed from the previous step's output.
            'data' => [
                'summary_text' => '@SummarizationStep.generated_text',
                'source_text_hash' => 'sha256_of_@initial.original_text' // A real system could hash inputs
            ],
            // The 'resource_type' is a static parameter for this step.
            'resource_type' => 'document_summary'
        ]
        // Output of this step: { "id": "summary_xyz789", "status": "SUCCESS" }
    ],
    [
        'id'   => 'VerifyStorageStep',
        'tool' => 'ReadRecord',
        'input_map' => [
            'id' => '@StoreSummaryStep.id', // Use the ID returned by the CreateRecord step
            'resource_type' => 'document_summary'
        ]
        // Output of this step: { "id": "...", "status": "FOUND", "data": { "summary_text": "..." } }
    ]
];

// Execute the pipeline
echo "Starting workflow to generate, store, and verify a summary...\n";
$result = Quicpro\PipelineOrchestrator::run($initialData, $pipelineDefinition);

// --- 3. Process the Final Result ---

if ($result->isSuccess()) {
    echo "\nPipeline completed successfully!\n";
    $creationOutput = $result->getOutputOfStep('StoreSummaryStep');
    $verificationOutput = $result->getOutputOfStep('VerifyStorageStep');

    echo "  - Summary generated by LLM.\n";
    echo "  - Summary stored successfully. New Record ID: " . ($creationOutput['id'] ?? 'N/A') . "\n";
    echo "  - Verification successful. Fetched record with ID " . ($verificationOutput['id'] ?? 'N/A') . ".\n";
    echo "  - Fetched Summary Text: \"" . ($verificationOutput['data']['summary_text'] ?? 'N/A') . "\"\n";

} else {
    echo "\nPipeline FAILED: " . $result->getErrorMessage() . "\n";
    $failedStepInfo = $result->getFailedStepDetails();
    if ($failedStepInfo) {
        echo "Failure occurred at step '{$failedStepInfo['step_id']}' with error: {$failedStepInfo['error']}\n";
    }
}
?>
