<?php
/*
 * This script initiates a pipeline to plan a new frontend module.
 * It leverages automated topic extraction for GraphRAG, fetches comprehensive context,
 * and then consults a specialized "Intelligent Intern" agent trained for module development.
 * Assumes the PipelineOrchestrator and tool handlers (ExtractTopicsFromRequest, GenerateText_ModuleDevIntern)
 * are configured in the application's bootstrap phase.
 * Automated context logging to a graph database is also assumed to be active.
 */

$userCodingRequest = "We want to build a new frontend module for user profile management. It should include features for editing personal details, managing notification preferences, and viewing activity history. The project uses Vue 3 and Pinia for state management.";

// Initial data for the pipeline, primarily the user's request.
$initialData = ['user_coding_task' => $userCodingRequest, 'project_identifier' => 'project_alpha'];

$pipelineDefinition = [
    [
        'tool' => 'ExtractTopicsFromRequest',
        // The orchestrator will automatically take initialData (if string) or a specified field
        // from initialData as input if 'input_map' is simple or by convention.
        // Here, we explicitly map it for clarity.
        'input_map' => ['text_input' => '@initial.user_coding_task']
        // Output of this step will be e.g., { "extracted_topics": ["frontend module", "user profile", "Vue 3", "Pinia"] }
    ],
    [
        'tool' => 'GenerateText_ModuleDevIntern', // Call the specialized "Intelligent Intern" for module development
        'input_map' => [
            // The main user request is passed directly to the intern.
            'user_request' => '@initial.user_coding_task',
            'project_id'   => '@initial.project_identifier'
        ],
        'params' => [
            'use_graph_context' => true, // Enable GraphRAG.
            /*
             * For GraphRAG context_topics:
             * The 'topics_from_previous_step_output' in the 'GenerateText_ModuleDevIntern'
             * tool handler configuration (set during bootstrap) will automatically use the 'extracted_topics'
             * from the 'ExtractTopicsFromRequest' step.
             */
            'context_depth' => 'auto',
            /*
             * 'context_depth' => 'auto': This is a special instruction to the GraphRAGAgent
             * (configured in the 'GenerateText_ModuleDevIntern' tool handler).
             * It signifies that the agent should retrieve a comprehensive context related to the
             * extracted topics (e.g., "frontend module", "user profile"). This involves fetching
             * all relevant information the system has ever stored and linked regarding module
             * construction, best practices, previously programmed modules, relevant Vue 3/Pinia patterns,
             * and any specific guidance previously given to or learned by "intern" agents.
             * The GraphRAGAgent translates "auto" into an extensive graph query.
             */
            'max_context_tokens' => 2000, // Allow a larger context for coding tasks.
            // 'model_identifier' => 'intern_v2' // Optionally specify a version of the intern agent.
            'llm_options' => ['guidance_level' => 'detailed'] // Custom options for the intern.
        ]
    ]
];

echo "Starting frontend module planning pipeline for request: \"{$userCodingRequest}\"\n";
$result = Quicpro\PipelineOrchestrator::run($initialData, $pipelineDefinition);

if ($result->isSuccess()) {
    echo "\nIntelligent Intern (Module Development) Response:\n";
    $internOutput = $result->getFinalOutput(); // Output of the last step (GenerateText_ModuleDevIntern)

    echo "Suggested Module Plan / Code Structure:\n";
    echo "----------------------------------------\n";
    echo ($internOutput['module_plan'] ?? 'No plan generated.') . "\n";
    echo "----------------------------------------\n";

    if (!empty($internOutput['important_notes'])) {
        echo "\nKey Considerations:\n";
        foreach ($internOutput['important_notes'] as $note) {
            echo "- " . $note . "\n";
        }
    }
    /*
     * The context used by the Intelligent Intern was automatically built by:
     * 1. The 'ExtractTopicsFromRequest' step identifying key concepts.
     * 2. The 'GenerateText_ModuleDevIntern' tool's RAG configuration (defined during bootstrap)
     * triggering the GraphRAGAgent.
     * 3. The GraphRAGAgent using the extracted topics and 'context_depth: "auto"' to perform
     * an extensive query against the graph database. This database contains knowledge
     * from all previous interactions, documentation, and specific training given to interns
     * regarding module development (e.g., hints on Vue 3, Pinia, common pitfalls,
     * architectural patterns from past module programming tasks).
     * 4. This rich, dynamically retrieved context was then provided to the
     * "IntelligentIntern_ModuleDevelopment" agent to enrich its planning process for the new module.
     * 5. All interactions (this pipeline run, its steps, inputs, outputs) are typically automatically
     * logged to the graph database by the PipelineOrchestrator's auto-context logging feature
     * (if enabled during bootstrap), further enriching the context for future tasks.
     */
} else {
    echo "\nPipeline Error: " . $result->getErrorMessage() . "\n";
    $failedStepInfo = $result->getFailedStepDetails();
    if ($failedStepInfo) {
        echo "Failure occurred at step ID '{$failedStepInfo['step_id']}' with error: {$failedStepInfo['error']}\n";
    }
}
?>