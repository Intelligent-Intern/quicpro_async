<?php
/*
 * Assumes the PipelineOrchestrator has been configured for 'GenerateText'
 * with RAG capabilities and for 'FetchUrlContent', 'ExtractTextFromHtml', 'ExtractKeywords'.
 * Also assumes php.ini provides 'quicpro.llm.default_model'.
 */

$urlToProcess = '[https://en.wikipedia.org/wiki/Retrieval-Augmented_Generation](https://en.wikipedia.org/wiki/Retrieval-Augmented_Generation)';

$initialData = ['target_url' => $urlToProcess];

$pipelineDefinition = [
    [
        'tool' => 'FetchUrlContent',
        'input_map' => ['url' => '@initial.target_url']
    ],
    [
        'tool' => 'ExtractTextFromHtml',
        'input_map' => ['html_content' => '@previous.body']
    ],
    [
        'tool' => 'ExtractKeywords', /* Output: { "keywords": ["string"] } */
        'input_map' => ['text_content' => '@previous.extracted_text'],
        'params' => ['max_keywords' => 5]
    ],
    [
        'tool' => 'GenerateText',
        /*
         * The 'PipelineOrchestrator' and the 'GenerateText' tool handler,
         * based on its 'rag_config', will:
         * 1. See 'use_graph_context' is true.
         * 2. See 'topics_from_previous_step_output' is configured to use 'keywords'
         * from the 'ExtractKeywords' step.
         * 3. Call the 'GraphRAGAgent' with these extracted keywords and 'context_depth'.
         * 4. Get 'retrieved_context_text' from the 'GraphRAGAgent'.
         * 5. Inject this context and the 'user_prompt' into the 'LLMBridgeRequest'.
         */
        'input_map' => ['user_prompt' => '@initial.question_about_url'], // The question is now in initialData
        'params' => [
            'use_graph_context'   => true,  // Enable RAG for this call
            'context_depth'       => 2,     // Abstract "hops" for GraphRAGAgent
            'max_context_tokens'  => 750,   // Max tokens for the retrieved context
            // 'model_identifier' => 'gpt-4-turbo' // Optionally override php.ini default
        ]
    ]
];

$initialData['question_about_url'] = "How does RAG help reduce hallucinations in LLMs, based on the content from {$urlToProcess} and related concepts?";

$result = Quicpro\PipelineOrchestrator::run($initialData, $pipelineDefinition);

if ($result->isSuccess()) {
    echo "LLM Response (with GraphRAG context):\n";
    echo wordwrap($result->getFinalOutput()['generated_text'] ?? 'No response received.', 100) . "\n";

    // For debugging, you might want to see the context that was fetched
    // This would require the orchestrator or RAG agent to also return it.
    // echo "\nRetrieved RAG context was:\n";
    // echo wordwrap($result->getIntermediateData('GenerateText_rag_context') ?? 'No RAG context logged.', 100) . "\n";
} else {
    echo "Pipeline Error: " . $result->getErrorMessage() . "\n";
}

?>