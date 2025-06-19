/*
 * include/cluster.h – Native Process Supervisor API for php-quicpro_async
 * =======================================================================
 *
 * This header file defines the public C API for the multi-worker process
 * supervisor integrated within the php-quicpro_async extension. It allows PHP
 * applications to spawn, manage, and monitor a robust pool of worker processes
 * using a C-native core for maximum performance and stability.
 *
 * Key Features Managed or Configurable via this API:
 * - Forking and supervision of multiple worker processes.
 * - Automatic restart of crashed workers with configurable policies.
 * - CPU core affinity for worker processes.
 * - Worker process priority (niceness) and advanced scheduling policies.
 * - Resource limits for workers (e.g., max open files).
 * - Optional privilege dropping for worker processes (UID/GID).
 * - Integration with Linux cgroups for resource isolation.
 * - User-defined PHP callables for worker lifecycle events and main logic.
 * - Graceful shutdown and inter-process signaling mechanisms.
 *
 * The C implementation ensures safe forking (typically before RINIT or in a
 * way that minimizes Zend engine conflicts in the master) and robust signal handling.
 * Argument information (arginfo) for these PHP_FUNCTIONs is in php_quicpro_arginfo.h.
 */

#ifndef QUICPRO_CLUSTER_H
#define QUICPRO_CLUSTER_H

#include <php.h>        /* Zend API, zval, PHP_FUNCTION */
#include <sys/types.h>  /* For uid_t, gid_t */
/* For scheduling constants like SCHED_OTHER, SCHED_FIFO, SCHED_RR, include <sched.h>
 * in cluster.c. These defines are for PHP userland to pass integer values.
 */

/* --- Constants for Scheduling Policies (for PHP userland) --- */
/* These values should correspond to SCHED_OTHER, SCHED_FIFO, SCHED_RR from <sched.h> */
#define QUICPRO_SCHED_OTHER 0
#define QUICPRO_SCHED_FIFO  1
#define QUICPRO_SCHED_RR    2

/* --- Cluster Orchestration Options --- */
/* This C struct is populated from the PHP options array passed to quicpro_cluster_orchestrate.
 * The C implementation should apply sensible defaults for any fields not explicitly set from PHP.
 */
typedef struct _quicpro_cluster_options_t {
    /* --- Worker Configuration --- */
    int num_workers;             /* Number of worker processes to spawn.
                                  * Default: Number of available CPU cores if set to 0 or not provided. */
    zend_bool enable_cpu_affinity; /* If true, attempts to pin workers to specific CPU cores (round-robin).
                                  * Default: false. Requires OS support and adequate permissions. */
    int worker_niceness;         /* Niceness value for worker processes (-20 for highest, 19 for lowest).
                                  * Default: 0 (kernel default). Requires privileges to set < 0. */
    int worker_scheduler_policy; /* Scheduling policy for workers (e.g., QUICPRO_SCHED_OTHER,
                                  * QUICPRO_SCHED_FIFO, QUICPRO_SCHED_RR).
                                  * Default: QUICPRO_SCHED_OTHER. Real-time policies require privileges. */
    long worker_max_open_files;  /* RLIMIT_NOFILE for worker processes (max open file descriptors).
                                  * Default: 0 (not changed, inherits from master). Set to a positive value to change. */
    char* worker_cgroup_path;    /* Optional: Path to an existing cgroup directory. Workers will be moved into this cgroup.
                                  * Default: NULL (no cgroup manipulation). Master needs write access to the cgroup's tasks file. */
    uid_t worker_uid;            /* Target UID for worker processes after fork.
                                  * Default: 0 (not changed, worker runs as master's UID). Requires master to be root for effective change. */
    gid_t worker_gid;            /* Target GID for worker processes after fork.
                                  * Default: 0 (not changed, worker runs as master's GID). Requires master to be root for effective change. */

    /* --- Worker Behavior Hint --- */
    int worker_loop_usleep_usec; /* Hint for PHP worker's main loop: microseconds to sleep if idle.
                                  * If 0, worker might busy-loop or yield (sched_yield). Default: 10000 (10ms).
                                  * The actual worker loop is implemented in PHP via worker_main_callable. */

    /* --- PHP Callbacks (passed as zval from PHP userland) --- */
    zval worker_main_callable;   /* REQUIRED: The main PHP callable that each worker process will execute.
                                  * This callable receives one argument: int $worker_id.
                                  * It should contain the worker's primary processing logic/loop. */
    zval on_worker_start_callable;/* Optional: PHP callable executed in the *master* process *after* a worker
                                  * is successfully forked and its environment (affinity, priority, etc.) is set up.
                                  * Receives: int $worker_id, int $worker_pid. */
    zval on_worker_exit_callable; /* Optional: PHP callable executed in the *master* process when a worker exits.
                                  * Receives: int $worker_id, int $worker_pid, int $exit_status, int $terminating_signal. */

    /* --- Master Supervisor Configuration --- */
    zend_bool restart_crashed_workers; /* If true, master supervisor restarts workers that terminate unexpectedly.
                                        * Default: true. */
    int max_restarts_per_worker;      /* Max number of restarts for a single worker slot within 'restart_interval_sec'.
                                        * Default: 5. Set to -1 for unlimited within interval. */
    int restart_interval_sec;         /* The time window (in seconds) during which 'max_restarts_per_worker' is effective.
                                        * Default: 60. */
    int graceful_shutdown_timeout_sec;/* Timeout (seconds) for workers to shut down gracefully after receiving SIGTERM
                                        * from the master, before master sends SIGKILL. Default: 30. */
    char* master_pid_file_path;       /* Optional: Path to a file where the master supervisor's PID will be written.
                                        * Default: NULL (no PID file written). */
    char* cluster_name;               /* Optional: A name for this cluster, useful for logging or identification if multiple clusters are run.
                                        * Default: "quicpro_cluster". */

} quicpro_cluster_options_t;

/*
 * PHP_FUNCTION(quicpro_cluster_orchestrate);
 * ------------------------------------------
 * Main entry point to initialize, spawn, and manage the worker cluster.
 * The C implementation of this function typically takes over the current PHP
 * process to become the master supervisor. It forks worker processes, which
 * then execute the 'worker_main_callable' from PHP userland.
 * The master supervisor monitors workers and handles restarts according to policy.
 * This function usually blocks and runs the supervisor loop until a shutdown
 * signal (e.g., SIGINT, SIGTERM) is received by the master process.
 *
 * Userland Signature (conceptual, defined in php_quicpro_arginfo.h):
 * bool quicpro_cluster_orchestrate(array $options)
 *
 * The $options array in PHP is parsed into the quicpro_cluster_options_t struct.
 * Missing options in the array will use C-level defaults.
 *
 * Returns: true if orchestration loop terminates normally (e.g., all workers exited cleanly
 * and no restart policy dictates further action), false on critical master
 * initialization error. Often, this function does not return to PHP if it
 * successfully enters the master supervision loop and is terminated by a signal.
 */
PHP_FUNCTION(quicpro_cluster_orchestrate);

/*
 * PHP_FUNCTION(quicpro_cluster_signal_workers);
 * ---------------------------------------------
 * Allows an external PHP script to send a signal to all worker processes
 * managed by an active cluster master. This usually requires identifying the
 * master process, e.g., via its PID file.
 *
 * Userland Signature:
 * bool quicpro_cluster_signal_workers(int $signal [, string $master_pid_file_path = null])
 *
 * $signal: The OS signal number to send (e.g., SIGTERM, SIGUSR1).
 * $master_pid_file_path: If provided, used to read the master process PID.
 * Alternatively, other IPC mechanisms could be used by the C impl.
 *
 * Returns: true if the signal dispatch was attempted successfully, false on error.
 */
PHP_FUNCTION(quicpro_cluster_signal_workers);

/*
 * PHP_FUNCTION(quicpro_cluster_get_stats);
 * ----------------------------------------
 * Retrieves statistics about the running cluster by communicating with the
 * active master supervisor process.
 *
 * Userland Signature:
 * array|false quicpro_cluster_get_stats([string $master_pid_file_path = null])
 *
 * Returns an associative array of cluster statistics (e.g., master PID,
 * number of active workers, individual worker status, restart counts, uptimes)
 * or false on error (e.g., master not found).
 */
PHP_FUNCTION(quicpro_cluster_get_stats);

#endif /* QUICPRO_CLUSTER_H */