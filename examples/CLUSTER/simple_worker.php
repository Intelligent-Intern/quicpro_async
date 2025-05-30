<?php
/* Example 1: Simple Worker with Basic Cluster Configuration */

if (!extension_loaded('quicpro')) {
    die("Quicpro extension not loaded.\n");
}

// Define the main logic for each worker
function my_simple_worker_main(int $workerId): void {
    $pid = getmypid();
    echo "[Worker {$workerId} PID: {$pid}] Started. Processing tasks for 10 seconds...\n";
    $startTime = time();
    while (time() - $startTime < 10) {
        // Simulate work
        usleep(500000); // 0.5 seconds
        echo "[Worker {$workerId} PID: {$pid}] ...working...\n";
    }
    echo "[Worker {$workerId} PID: {$pid}] Finished and exiting.\n";
    // Worker exits when this function returns or script ends
}

// Define a callback for when the master supervisor starts a worker
function my_on_master_worker_start(int $workerId, int $workerPid): void {
    echo "[Master Supervisor] Worker {$workerId} (PID: {$workerPid}) has been started.\n";
}

// Define a callback for when the master supervisor detects a worker has exited
function my_on_master_worker_exit(int $workerId, int $workerPid, int $exitStatus, int $termSignal): void {
    echo "[Master Supervisor] Worker {$workerId} (PID: {$workerPid}) exited. Status: {$exitStatus}, Signal: {$termSignal}.\n";
}

$supervisorOptions = [
    'num_workers' => 2, // Start 2 worker processes
    'worker_main_callable' => 'my_simple_worker_main', // Each worker runs this PHP function
    'on_worker_start_callable' => 'my_on_master_worker_start', // Optional: Master's callback on worker start
    'on_worker_exit_callable' => 'my_on_master_worker_exit',   // Optional: Master's callback on worker exit
    'cluster_name' => 'SimpleTestCluster',
    // All other options (affinity, priority, restarts, timeouts) use C-level defaults
    // For example, 'restart_crashed_workers' would default to true.
];

echo "[Master Supervisor] Starting SimpleTestCluster with 2 workers...\n";
// This call will typically block and run the C-native master supervisor loop.
// It only returns if all workers exit and restart is disabled, or on critical master error.
$result = Quicpro\Cluster::orchestrate($supervisorOptions);

if ($result) {
    echo "[Master Supervisor] Orchestration ended normally.\n";
} else {
    echo "[Master Supervisor] Orchestration failed to initialize or exited with error.\n";
}
