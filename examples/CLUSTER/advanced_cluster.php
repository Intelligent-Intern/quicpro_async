<?php
/* Example 2: Simple Worker with Expert Cluster Configuration */

if (!extension_loaded('quicpro')) {
    die("Quicpro extension not loaded.\n");
}

// Worker logic (same as Example 1 for simplicity)
function my_advanced_env_worker_main(int $workerId): void {
    $pid = getmypid();
    $niceness = proc_get_priority(0, $pid, PRIO_PROCESS); // Check niceness
    echo "[Worker {$workerId} PID: {$pid}] Started. Niceness: {$niceness}. Processing for 5 seconds...\n";
    sleep(5);
    echo "[Worker {$workerId} PID: {$pid}] Finished and exiting.\n";
}

function my_expert_on_master_worker_start(int $workerId, int $workerPid): void {
    echo "[Master Supervisor] Expert Worker {$workerId} (PID: {$workerPid}) has been configured and started.\n";
}

function my_expert_on_master_worker_exit(int $workerId, int $workerPid, int $exitStatus, int $termSignal): void {
    echo "[Master Supervisor] Expert Worker {$workerId} (PID: {$workerPid}) exited. Status: {$exitStatus}, Signal: {$termSignal}.\n";
}

$supervisorOptions = [
    'num_workers' => 1, // Just one worker for this demo to observe settings
    'worker_main_callable' => 'my_advanced_env_worker_main',
    'on_worker_start_callable' => 'my_expert_on_master_worker_start',
    'on_worker_exit_callable' => 'my_expert_on_master_worker_exit',

    // Expert configurations
    'enable_cpu_affinity' => true,       // Attempt to pin worker (OS dependent, might need specific core count)
    'worker_niceness' => -10,            // Higher priority (requires privileges)
    'worker_scheduler_policy' => QUICPRO_SCHED_RR, // Real-time round-robin (requires privileges)
    'worker_max_open_files' => 8192,     // Set RLIMIT_NOFILE for the worker
    // 'worker_cgroup_path' => '/sys/fs/cgroup/my_php_workers', // Example cgroup path (must exist and be writable by master)
    // 'worker_uid' => 1000, // Drop privileges to user 1000 (master must be root)
    // 'gid_t worker_gid' => 1000, // Drop privileges to group 1000 (master must be root)
    'worker_loop_usleep_usec' => 0,      // Hint for worker to potentially busy-loop or yield for lowest latency

    'restart_crashed_workers' => true,
    'max_restarts_per_worker' => 3,
    'restart_interval_sec' => 30,
    'graceful_shutdown_timeout_sec' => 10,
    'master_pid_file_path' => '/tmp/expert_cluster.pid',
    'cluster_name' => 'ExpertConfigCluster',
];

echo "[Master Supervisor] Starting ExpertConfigCluster with advanced settings...\n";
$result = Quicpro\Cluster::orchestrate($supervisorOptions);

if ($result) {
    echo "[Master Supervisor] Orchestration ended normally.\n";
} else {
    echo "[Master Supervisor] Orchestration failed to initialize or exited with error.\n";
}
