<?php
/* Example 4: Worker with Demanding PipelineOrchestrator Task & Expert Cluster Config */

if (!extension_loaded('quicpro')) {
    die("Quicpro extension not loaded.\n");
}

// Assume Quicpro\PipelineOrchestrator, Quicpro\Proto are available.
// Assume HFT-specific tool handlers ('FetchMarketDataHFT', 'ExecuteOrderHFT', 'ConditionalLogic')
// and Proto schemas are registered in a bootstrap phase available to workers.

function my_hft_pipeline_worker_main(int $workerId): void {
    $pid = getmypid();
    echo "[HFTPipelineWorker {$workerId} PID: {$pid}] Started. Ready for HFT pipeline execution.\n";

    // Simulate receiving a trading signal or market event
    $instrumentSymbol = 'EURUSD_FX';
    $clientOrderId = "W{$workerId}_ORD_" . time();
    $initialHftData = [
        'target_instrument' => $instrumentSymbol,
        'client_order_id' => $clientOrderId,
        'buy_price_threshold' => 1.07500 // Worker-specific or dynamically fetched threshold
    ];

    // Identical HFT pipeline definition from previous examples
    $hftPipelineDefinition = [
        ['tool' => 'FetchMarketDataHFT', 'input_map' => ['instrument' => '@initial.target_instrument']],
        ['tool' => 'ConditionalLogic', 'input_map' => ['condition_field_path' => '@previous.ask', 'value_to_compare' => '@initial.buy_price_threshold'], 'params' => ['operator' => 'lt']],
        ['tool' => 'ExecuteOrderHFT', 'condition_true_only' => true, 'input_map' => ['instrument' => '@FetchMarketDataHFT.instrument', 'trade_side' => 'BUY', 'limit_price'=> '@FetchMarketDataHFT.ask', 'order_qty'  => 50000, 'order_ref'  => '@initial.client_order_id'], 'params' => ['type' => 'LIMIT']]
    ];

    $pipelineExecutionOptions = [
        'overall_timeout_ms' => 150, // Very aggressive for this worker's pipeline run
        'fail_fast' => true,
    ];

    try {
        $result = Quicpro\PipelineOrchestrator::run($initialHftData, $hftPipelineDefinition, $pipelineExecutionOptions);
        if ($result->isSuccess()) {
            $orderConfirmation = $result->getOutputOfStep('ExecuteOrderHFT');
            if ($orderConfirmation) {
                echo "[HFTPipelineWorker {$workerId}] Order Result: " . ($orderConfirmation['exec_status'] ?? 'N/A') . "\n";
            } else {
                echo "[HFTPipelineWorker {$workerId}] Order condition not met or no order placed.\n";
            }
        } else {
            echo "[HFTPipelineWorker {$workerId}] HFT Pipeline Error: " . $result->getErrorMessage() . "\n";
        }
    } catch (\Throwable $e) {
        echo "[HFTPipelineWorker {$workerId}] CRITICAL ERROR in HFT task: " . $e->getMessage() . "\n";
    }
    echo "[HFTPipelineWorker {$workerId} PID: {$pid}] HFT task cycle finished.\n";
    // In a real HFT worker, this would be a loop or event-driven.
}


$supervisorOptions = [
    'num_workers' => 2, // e.g., 2 dedicated HFT pipeline workers
    'worker_main_callable' => 'my_hft_pipeline_worker_main',
    'on_worker_start_callable' => fn($wid, $pid) => print "[Master] HFT Worker {$wid} (PID {$pid}) started.\n",
    'on_worker_exit_callable' => fn($wid, $pid, $st, $sg) => print "[Master] HFT Worker {$wid} (PID {$pid}) exited. St:{$st} Sg:{$sg}\n",

    // Expert configurations for HFT environment
    'enable_cpu_affinity' => true,
    'worker_niceness' => -15, // High priority
    'worker_scheduler_policy' => QUICPRO_SCHED_FIFO, // Real-time FIFO (requires privileges)
    'worker_max_open_files' => 16384,
    // 'worker_cgroup_path' => '/sys/fs/cgroup/critical_services/hft_workers', // Ensure this path exists
    'worker_loop_usleep_usec' => 0, // Hint for workers to use busy-polling or sched_yield for lowest latency

    'restart_crashed_workers' => true, // Critical HFT workers should restart
    'max_restarts_per_worker' => 10,   // Allow more restarts for critical tasks
    'restart_interval_sec' => 60,
    'graceful_shutdown_timeout_sec' => 5, // Quick shutdown
    'master_pid_file_path' => '/var/run/hft_cluster.pid',
    'cluster_name' => 'HFTPipelineCluster',
];

echo "[Master Supervisor] Starting HFTPipelineCluster with expert settings...\n";
$result = Quicpro\Cluster::orchestrate($supervisorOptions); // This blocks

if ($result) {
    echo "[Master Supervisor] Orchestration ended normally.\n";
} else {
    echo "[Master Supervisor] Orchestration failed to initialize or exited with error.\n";
}
?>