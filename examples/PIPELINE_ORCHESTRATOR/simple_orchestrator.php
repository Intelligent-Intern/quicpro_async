<?php
/* Example 3: Worker with Simple PipelineOrchestrator Task & Simple Cluster Config */

if (!extension_loaded('quicpro')) {
    die("Quicpro extension not loaded.\n");
}

// Assume Quicpro\PipelineOrchestrator and Quicpro\Proto are available
// and tool handlers (e.g., for 'GenerateText' pointing to an LLM bridge) are registered.
// This registration would happen in a shared bootstrap file included by the master
// before forking, or by each worker if they don't inherit the master's PHP state perfectly.
// For simplicity, we assume it's available to the worker.

function my_orchestrator_worker_main(int $workerId): void {
    $pid = getmypid();
    echo "[OrchestratorWorker {$workerId} PID: {$pid}] Started. Will process one LLM query.\n";

    // This bootstrap would typically run once when the worker starts, or be inherited.
    // Ensure Tool Handlers are registered (conceptual - actual registration in bootstrap)
    // Quicpro\PipelineOrchestrator::registerToolHandler('GenerateText', [/* ...config... */]);

    $userPrompt = "What is QUIC?";
    $pipelineDefinition = [
        ['tool' => 'GenerateText', 'params' => ['model_identifier' => 'default_fast_model']]
        // Assumes 'default_fast_model' is known by the 'GenerateText' tool handler config
        // or php.ini 'quicpro.llm.default_model' is set appropriately.
    ];

    try {
        $result = Quicpro\PipelineOrchestrator::run($userPrompt, $pipelineDefinition);
        if ($result->isSuccess()) {
            $llmResponse = $result->getFinalOutput()['generated_text'] ?? 'No text generated.';
            echo "[OrchestratorWorker {$workerId} PID: {$pid}] LLM Response: " . substr($llmResponse, 0, 70) . "...\n";
        } else {
            echo "[OrchestratorWorker {$workerId} PID: {$pid}] Pipeline Error: " . $result->getErrorMessage() . "\n";
        }
    } catch (\Throwable $e) {
        echo "[OrchestratorWorker {$workerId} PID: {$pid}] CRITICAL ERROR in orchestrator task: " . $e->getMessage() . "\n";
    }

    echo "[OrchestratorWorker {$workerId} PID: {$pid}] Task finished, exiting after a short delay.\n";
    sleep(2); // Keep worker alive briefly to see output
}

$supervisorOptions = [
    'num_workers' => 1,
    'worker_main_callable' => 'my_orchestrator_worker_main',
    'on_worker_exit_callable' => function(int $wid, int $pid, int $status, int $sig) {
        echo "[Master] OrchestratorWorker {$wid} (PID {$pid}) exited. Status: {$status}, Signal: {$sig}\n";
    },
    'cluster_name' => 'SimpleOrchestratorCluster',
    // Default C-level options for restarts, etc.
];

echo "[Master Supervisor] Starting SimpleOrchestratorCluster...\n";
$result = Quicpro\Cluster::orchestrate($supervisorOptions);

if ($result) {
    echo "[Master Supervisor] Orchestration ended normally.\n";
} else {
    echo "[Master Supervisor] Orchestration failed to initialize or exited with error.\n";
}
