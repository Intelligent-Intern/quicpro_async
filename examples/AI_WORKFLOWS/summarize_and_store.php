<?php
/**
 * Simple "Noob" Wrapper for the Summarize-and-Store Workflow
 *
 * This example demonstrates how the complex pipeline definition from the previous
 * example can be wrapped into a single, easy-to-use function. This provides a
 * simple API for common tasks, while still being powered by the C-native
 * PipelineOrchestrator under the hood.
 */

// This function would typically live in a helper file or a service class.
// It requires that the 'GenerateText' and 'CreateRecord' tools have already
// been registered with the PipelineOrchestrator in a bootstrap phase.

/**
 * Generates a summary for a given text and stores it using a predefined workflow.
 *
 * @param string $textToSummarize The text to be processed.
 * @return string|false The ID of the newly created record on success, or false on failure.
 */
function summarizeAndStoreText(string $textToSummarize): string|false
{
    // The complex pipeline definition is hidden inside this wrapper function.
    $pipelineDefinition = [
        [
            'id'   => 'SummarizationStep',
            'tool' => 'GenerateText',
            'input_map' => [
                'prompt' => "Summarize the following text in one short sentence: '{{@initial.text}}'"
            ]
        ],
        [
            'id'   => 'StoreSummaryStep',
            'tool' => 'CreateRecord',
            'input_map' => [
                'resource_type' => 'document_summary',
                'data' => [
                    'summary_text' => '@SummarizationStep.generated_text',
                    'source_hash'  => '@initial.text_hash' // Pass hash instead of full text
                ]
            ]
        ]
    ];

    // The initial data for the pipeline.
    $initialData = [
        'text' => $textToSummarize,
        'text_hash' => hash('sha256', $textToSummarize)
    ];

    // The entire multi-step, multi-agent workflow is executed with a single call.
    $result = Quicpro\PipelineOrchestrator::run($initialData, $pipelineDefinition);

    // The wrapper handles the result and returns a simple, useful value.
    if ($result->isSuccess()) {
        $storageOutput = $result->getOutputOfStep('StoreSummaryStep');
        return $storageOutput['id'] ?? false;
    }

    // Log the error for developers, but return a simple false to the user.
    // error_log("Summarize-and-store workflow failed: " . $result->getErrorMessage());
    return false;
}

/**********************************************************************************************************************/

// --- USER CODE ---

// Assume the `summarizeAndStoreText` function is available via autoloading or require.
$originalText = "QUIC (Quick UDP Internet Connections) is a new transport protocol that reduces latency compared to TCP. It is encrypted by default using TLS 1.3 and handles multiplexing without head-of-line blocking. It was standardized by the IETF and is the foundation of HTTP/3.";

echo "Processing text and storing summary...\n";

// The user only needs to call one simple, descriptive function.
$newRecordId = summarizeAndStoreText($originalText);

if ($newRecordId) {
    echo "Workflow successful! Summary stored with new Record ID: " . $newRecordId . "\n";
} else {
    echo "Workflow failed. Please check the logs for more details.\n";
}

