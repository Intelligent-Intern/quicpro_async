<?php
/**
 * MCP Agent: Requirement Analysis Service (Distributed Inference Version)
 *
 * This version demonstrates how an agent can leverage a massive language model
 * that is sharded across multiple smaller machines (a "Mixture of Experts" or
 * model parallelism approach).
 *
 * The agent's "thought process" is defined as a pipeline that the C-native
 * PipelineOrchestrator executes across the distributed inference agents.
 */

use Quicpro\Config;
use Quicpro\MCP\Server as McpServer;
use Quicpro\PipelineOrchestrator;
use Quicpro\IIBIN;

// --- 1. Bootstrap: Define All Necessary Schemas ---

function register_distributed_schemas(): void
{
    // For this agent's own service
    IIBIN::defineSchema('RequirementAnalysisRequest', ['user_request' => ['type' => 'string', 'tag' => 1]]);
    IIBIN::defineSchema('RequirementAnalysisResponse', [
        'task_summary'           => ['type' => 'string', 'tag' => 1],
        'is_ambiguous'           => ['type' => 'bool',   'tag' => 2],
        'clarification_question' => ['type' => 'string', 'tag' => 3, 'optional' => true],
    ]);

    // For communication between the distributed inference agents/shards
    // These schemas handle the flow of intermediate model states.
    IIBIN::defineSchema('InitialPrompt', ['prompt' => ['type' => 'string', 'tag' => 1]]);
    IIBIN::defineSchema('IntermediateState', ['hidden_state' => ['type' => 'bytes', 'tag' => 1]]);
    IIBIN::defineSchema('FinalResult', ['generated_text' => ['type' => 'string', 'tag' => 1]]);
}


// --- 2. Service Handler: The Core Logic of the Agent ---

class AnalysisService
{
    /**
     * This method is the entry point for the tool. It doesn't call an LLM directly.
     * Instead, it constructs a distributed pipeline definition and hands it off
     * to the C-native PipelineOrchestrator to execute.
     *
     * @param string $requestPayloadBinary The incoming IIBIN binary payload.
     * @return string The IIBIN binary response payload.
     */
    public function analyze(string $requestPayloadBinary): string
    {
        try {
            $requestData = IIBIN::decode('RequirementAnalysisRequest', $requestPayloadBinary);
            $userRequest = $requestData['user_request'];

            // Define the "thought process" as a distributed pipeline.
            // The orchestrator will execute this across multiple machines.
            $pipelineDefinition = [
                [
                    'id'   => 'EmbeddingAndRouting',
                    'tool' => 'DistributedModelRouter', // This tool determines which experts to use.
                    'input_map' => ['prompt' => '@initial.user_request']
                    // Output: { "experts_to_use": ["ExpertAgent_Code", "ExpertAgent_Logic"] }
                ],
                [
                    'id'   => 'ExpertCoderExecution',
                    'tool' => 'ExpertAgent_Code', // A specific shard of the 400B model
                    'input_map' => ['prompt' => '@initial.user_request']
                    // Output: { "code_analysis_state": "bytes" }
                ],
                [
                    'id'   => 'ExpertLogicExecution',
                    'tool' => 'ExpertAgent_Logic', // Another shard of the 400B model
                    'input_map' => ['prompt' => '@initial.user_request']
                    // Output: { "logic_analysis_state": "bytes" }
                ],
                [
                    'id'   => 'SynthesisStep',
                    'tool' => 'FinalOutputSynthesizer', // The final layer/shard of the model
                    'input_map' => [
                        'code_state'  => '@ExpertCoderExecution.code_analysis_state',
                        'logic_state' => '@ExpertLogicExecution.logic_analysis_state',
                        'original_prompt' => '@initial.user_request'
                    ]
                    // Output: { "generated_text": "..." } (The final JSON response)
                ]
            ];

            // Run the entire distributed pipeline. The C-native orchestrator handles
            // the parallel/sequential execution, MCP communication, and data flow.
            echo "[AnalysisAgent] Initiating distributed inference pipeline for request...\n";
            $result = PipelineOrchestrator::run(['user_request' => $userRequest], $pipelineDefinition);

            if (!$result->isSuccess()) {
                throw new \RuntimeException("Distributed inference pipeline failed: " . $result->getErrorMessage());
            }

            // The final result comes from the last step in the pipeline.
            $synthesisOutput = $result->getOutputOfStep('SynthesisStep');
            $analysisJson = $synthesisOutput['generated_text'] ?? '{}';
            $analysisResult = json_decode($analysisJson, true);

            return IIBIN::encode('RequirementAnalysisResponse', [
                'task_summary'           => $analysisResult['task_summary'] ?? 'N/A',
                'is_ambiguous'           => (bool)($analysisResult['is_ambiguous'] ?? true),
                'clarification_question' => $analysisResult['clarification_question'] ?? 'Could you please provide more details?',
            ]);

        } catch (\Throwable $e) {
            error_log("[AnalysisAgent] ERROR: " . $e->getMessage());
            return IIBIN::encode('RequirementAnalysisResponse', [
                'task_summary' => 'Failed to analyze request.',
                'is_ambiguous' => true,
                'clarification_question' => "An internal error occurred in the distributed pipeline: " . $e->getMessage(),
            ]);
        }
    }
}


// --- 3. Main Server Execution ---
// This part of the script starts the MCP server for this AnalysisAgent.
// Its tool handlers for 'DistributedModelRouter', 'ExpertAgent_Code', etc.,
// would be registered in a global bootstrap file.

echo "[AnalysisAgent] Starting up...\n";
register_distributed_schemas();
$serverConfig = Config::new([ /* ... server config ... */ ]);

try {
    $server = new McpServer('0.0.0.0', 7100, $serverConfig);
    $analysisHandler = new AnalysisService();
    $server->registerService('AnalysisService', $analysisHandler);
    echo "[AnalysisAgent] Service 'AnalysisService' registered. Listening on port 7100...\n";
    $server->run(); // This is a blocking call that starts the server loop.
} catch (\Throwable $e) {
    die("[AnalysisAgent] FATAL ERROR during startup: " . $e->getMessage() . "\n");
}
