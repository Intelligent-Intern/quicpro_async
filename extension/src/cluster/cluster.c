/*
 * src/cluster.c – C-Native Process Supervisor Implementation
 * ==========================================================
 *
 * Implements the C-native logic for the multi-worker process supervisor.
 * The core function, `quicpro_cluster_orchestrate`, parses PHP options,
 * forks worker processes, sets up their execution environment (affinity, priority, etc.),
 * and enters a supervision loop to monitor and restart workers.
 * It also handles graceful shutdown via OS signals.
 */

#include "php_quicpro.h"
#include "cluster.h"
#include "cancel.h" /* For throwing exceptions */

#include <unistd.h>     /* for fork, getpid, getmypid, setuid, setgid, usleep */
#include <signal.h>     /* for signal, kill */
#include <sys/wait.h>   /* for waitpid */
#include <sys/resource.h> /* for setrlimit */
#include <sched.h>      /* for sched_setaffinity, sched_setscheduler */
#include <string.h>     /* for strerror */
#include <errno.h>      /* for errno */
#include <time.h>       /* for time() */
#include <stdio.h>      /* for FILE, fopen, fprintf, fclose */

/* --- Master Process Global State --- */
typedef struct {
    pid_t pid;
    int worker_id;
    time_t start_time;
    int restart_count;
    time_t last_restart_time;
    zend_bool is_exiting;
} quicpro_worker_info_t;

static quicpro_worker_info_t *g_worker_pool = NULL;
static int g_num_workers = 0;
static zval g_on_worker_exit_callable;

/* Volatile for signal safety */
static volatile sig_atomic_t g_shutdown_request = 0; /* SIGINT/SIGTERM received */
static volatile sig_atomic_t g_reload_request = 0;   /* SIGHUP received */


/* --- Static Helper Function Prototypes --- */
static int parse_options_from_php(zval *php_options_array, quicpro_cluster_options_t *c_options);
static void cleanup_c_options(quicpro_cluster_options_t *c_options);
static void master_supervisor_loop(quicpro_cluster_options_t *c_options);
static pid_t fork_and_start_worker(quicpro_cluster_options_t *c_options, int worker_id);
static void worker_process_main(quicpro_cluster_options_t *c_options, int worker_id);
static void cluster_signal_handler(int signo);
static void write_pid_file(const char *path);
static void remove_pid_file(const char *path);

/* --- PHP_FUNCTION Implementations --- */

PHP_FUNCTION(quicpro_cluster_orchestrate)
{
    zval *php_options_array;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY(php_options_array)
    ZEND_PARSE_PARAMETERS_END();

    quicpro_cluster_options_t c_options = {0}; /* Zero-initialize */

    /* Parse PHP array into our C options struct */
    if (parse_options_from_php(php_options_array, &c_options) == FAILURE) {
        /* Error already thrown by parser */
        cleanup_c_options(&c_options);
        RETURN_FALSE;
    }

    /* Write PID file if requested */
    if (c_options.master_pid_file_path) {
        write_pid_file(c_options.master_pid_file_path);
    }

    /* Setup signal handlers for the master process */
    signal(SIGTERM, cluster_signal_handler);
    signal(SIGINT, cluster_signal_handler);
    signal(SIGHUP, cluster_signal_handler);
    signal(SIGCHLD, SIG_DFL); /* Let waitpid handle it */

    /* Allocate global worker pool */
    g_num_workers = c_options.num_workers;
    g_worker_pool = ecalloc(g_num_workers, sizeof(quicpro_worker_info_t));

    /* Store callbacks globally for use in signal handlers/master loop */
    if (Z_TYPE(c_options.on_worker_exit_callable) != IS_UNDEF) {
        ZVAL_COPY(&g_on_worker_exit_callable, &c_options.on_worker_exit_callable);
    } else {
        ZVAL_UNDEF(&g_on_worker_exit_callable);
    }

    /* Initial fork of all workers */
    for (int i = 0; i < g_num_workers; ++i) {
        pid_t pid = fork_and_start_worker(&c_options, i);
        if (pid < 0) {
            /* Forking failed, kill any children we already made and exit */
            for (int j = 0; j < i; ++j) {
                if (g_worker_pool[j].pid > 0) kill(g_worker_pool[j].pid, SIGKILL);
            }
            throw_mcp_error_as_php_exception(0, "Failed to fork worker process #%d: %s", i, strerror(errno));
            goto cleanup_and_fail;
        }
        g_worker_pool[i] = (quicpro_worker_info_t){ .pid = pid, .worker_id = i, .start_time = time(NULL), .last_restart_time = time(NULL), .restart_count = 0, .is_exiting = 0 };

        /* Call the on_worker_start PHP callback in the master process */
        if (Z_TYPE(c_options.on_worker_start_callable) != IS_UNDEF) {
            zval args[2], retval;
            ZVAL_LONG(&args[0], i);
            ZVAL_LONG(&args[1], pid);
            call_user_function_ex(NULL, NULL, &c_options.on_worker_start_callable, &retval, 2, args, 0, NULL);
            zval_ptr_dtor(&retval);
        }
    }

    /* Enter the main supervisor loop. This function typically only exits on shutdown signal. */
    master_supervisor_loop(&c_options);

    /* --- Shutdown Sequence --- */
    php_printf("[Master Supervisor] Shutdown initiated. Sending SIGTERM to all workers...\n");
    for (int i = 0; i < g_num_workers; ++i) {
        if (g_worker_pool[i].pid > 0) {
            kill(g_worker_pool[i].pid, SIGTERM);
        }
    }

    /* Wait for graceful shutdown timeout */
    time_t shutdown_deadline = time(NULL) + c_options.graceful_shutdown_timeout_sec;
    while (time(NULL) < shutdown_deadline) {
        pid_t exited_pid = waitpid(-1, NULL, WNOHANG);
        if (exited_pid <= 0) {
            usleep(100000); /* 100ms sleep */
            continue;
        }
        int all_exited = 1;
        for (int i = 0; i < g_num_workers; ++i) {
            if (g_worker_pool[i].pid == exited_pid) g_worker_pool[i].pid = 0;
            if (g_worker_pool[i].pid > 0) all_exited = 0;
        }
        if (all_exited) break;
    }

    php_printf("[Master Supervisor] Graceful shutdown period ended. Sending SIGKILL to any remaining workers...\n");
    for (int i = 0; i < g_num_workers; ++i) {
        if (g_worker_pool[i].pid > 0) {
            kill(g_worker_pool[i].pid, SIGKILL);
        }
    }

cleanup_and_fail:
    if (c_options.master_pid_file_path) {
        remove_pid_file(c_options.master_pid_file_path);
    }
    cleanup_c_options(&c_options);
    if (g_worker_pool) efree(g_worker_pool);
    if (Z_TYPE(g_on_worker_exit_callable) != IS_UNDEF) zval_ptr_dtor(&g_on_worker_exit_callable);

    RETURN_TRUE;
}

PHP_FUNCTION(quicpro_cluster_signal_workers)
{
    /* TODO: Requires IPC. Simplest implementation using PID file: */
    zend_long signal;
    char *pid_file_path = NULL;
    size_t pid_file_path_len;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_LONG(signal)
        Z_PARAM_OPTIONAL
        Z_PARAM_STRING(pid_file_path, pid_file_path_len)
    ZEND_PARSE_PARAMETERS_END();

    if (!pid_file_path) {
        throw_mcp_error_as_php_exception(0, "PID file path must be provided to signal workers.");
        RETURN_FALSE;
    }
    FILE *f = fopen(pid_file_path, "r");
    if (!f) {
        throw_mcp_error_as_php_exception(0, "Could not open master PID file: %s", pid_file_path);
        RETURN_FALSE;
    }
    pid_t master_pid;
    if (fscanf(f, "%d", &master_pid) != 1) {
        fclose(f);
        throw_mcp_error_as_php_exception(0, "Could not read PID from master PID file: %s", pid_file_path);
        RETURN_FALSE;
    }
    fclose(f);

    /* Send the signal to the master process, which will then forward it to workers. */
    if (kill(master_pid, (int)signal) != 0) {
        throw_mcp_error_as_php_exception(0, "Failed to send signal %ld to master process %d: %s", signal, master_pid, strerror(errno));
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

PHP_FUNCTION(quicpro_cluster_get_stats)
{
    /* TODO: Requires IPC. A robust implementation would use shared memory or a unix domain socket
     * that the master process listens on for stat requests.
     */
    zend_error(E_WARNING, "quicpro_cluster_get_stats() is not yet implemented.");
    RETURN_FALSE;
}


/* --- C Helper Implementations --- */

/* Main supervision loop for the master process. */
static void master_supervisor_loop(quicpro_cluster_options_t *c_options) {
    php_printf("[Master Supervisor] Entering main supervision loop...\n");
    while (!g_shutdown_request) {
        if (g_reload_request) {
            php_printf("[Master Supervisor] SIGHUP received. Sending SIGTERM to all workers for graceful reload...\n");
            for (int i = 0; i < g_num_workers; ++i) {
                if (g_worker_pool[i].pid > 0) {
                    kill(g_worker_pool[i].pid, SIGTERM);
                    g_worker_pool[i].is_exiting = 1; /* Mark as intentionally stopping */
                }
            }
            g_reload_request = 0; /* Reset flag */
        }

        int status;
        pid_t child_pid = waitpid(-1, &status, WNOHANG);
        if (child_pid <= 0) {
            /* No child exited, or an error occurred. Sleep briefly to prevent busy-looping. */
            usleep(50000); /* 50ms */
            continue;
        }

        /* A child process has exited. Find it in our pool. */
        int worker_id = -1;
        for (int i = 0; i < g_num_workers; ++i) {
            if (g_worker_pool[i].pid == child_pid) {
                worker_id = i;
                break;
            }
        }
        if (worker_id == -1) continue; /* Not one of our direct children? Ignore. */

        int exit_code = 0, term_signal = 0;
        if (WIFEXITED(status)) exit_code = WEXITSTATUS(status);
        if (WIFSIGNALED(status)) term_signal = WTERMSIG(status);

        /* Call the on_worker_exit PHP callback */
        if (Z_TYPE(g_on_worker_exit_callable) != IS_UNDEF) {
            zval args[4], retval;
            ZVAL_LONG(&args[0], worker_id);
            ZVAL_LONG(&args[1], child_pid);
            ZVAL_LONG(&args[2], exit_code);
            ZVAL_LONG(&args[3], term_signal);
            call_user_function_ex(NULL, NULL, &g_on_worker_exit_callable, &retval, 4, args, 0, NULL);
            zval_ptr_dtor(&retval);
        }

        /* Handle restart logic */
        if (c_options->restart_crashed_workers && !g_worker_pool[worker_id].is_exiting && !g_shutdown_request) {
            time_t now = time(NULL);
            if (now - g_worker_pool[worker_id].last_restart_time > c_options->restart_interval_sec) {
                g_worker_pool[worker_id].restart_count = 0; // Reset restart count after interval
            }
            g_worker_pool[worker_id].restart_count++;
            g_worker_pool[worker_id].last_restart_time = now;

            if (c_options->max_restarts_per_worker < 0 || g_worker_pool[worker_id].restart_count <= c_options->max_restarts_per_worker) {
                php_printf("[Master Supervisor] Worker %d (PID %d) exited unexpectedly. Restarting... (Attempt %d)\n", worker_id, child_pid, g_worker_pool[worker_id].restart_count);
                pid_t new_pid = fork_and_start_worker(c_options, worker_id);
                if (new_pid > 0) {
                     g_worker_pool[worker_id].pid = new_pid;
                     g_worker_pool[worker_id].start_time = time(NULL);
                } else {
                     php_error(E_WARNING, "[Master Supervisor] Failed to restart worker %d: %s", worker_id, strerror(errno));
                     g_worker_pool[worker_id].pid = 0; // Mark as dead
                }
            } else {
                php_error(E_WARNING, "[Master Supervisor] Worker %d (PID %d) exceeded max restart limit. Not restarting.", worker_id, child_pid);
                 g_worker_pool[worker_id].pid = 0;
            }
        } else {
             g_worker_pool[worker_id].pid = 0; /* Mark as dead, no restart */
        }
    }
}

/* Forks a single worker and sets up its environment */
static pid_t fork_and_start_worker(quicpro_cluster_options_t *c_options, int worker_id) {
    pid_t pid = fork();

    if (pid < 0) { /* Fork failed */
        return -1;
    }

    if (pid == 0) { /* --- In Child Worker Process --- */
        worker_process_main(c_options, worker_id);
        /* worker_process_main should call exit(), but we add one here for safety */
        exit(0);
    }

    /* --- In Parent Master Process --- */
    return pid;
}

/* The main function for the forked child process */
static void worker_process_main(quicpro_cluster_options_t *c_options, int worker_id) {
    /* Child should have its own signal handling, often reset to defaults */
    signal(SIGTERM, SIG_DFL);
    signal(SIGINT, SIG_DFL);
    signal(SIGHUP, SIG_DFL);

    /* Set CPU Affinity if enabled */
    if (c_options->enable_cpu_affinity) {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(worker_id % sysconf(_SC_NPROCESSORS_ONLN), &cpuset); // Simple round-robin affinity
        if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) != 0) {
            php_error(E_WARNING, "[Worker %d] Failed to set CPU affinity: %s", worker_id, strerror(errno));
        }
    }

    /* Set Scheduling Policy and Priority (Niceness) */
    if (c_options->worker_scheduler_policy != QUICPRO_SCHED_OTHER) {
        struct sched_param sp = { .sched_priority = sched_get_priority_min(c_options->worker_scheduler_policy) };
        if (sched_setscheduler(0, c_options->worker_scheduler_policy, &sp) != 0) {
            php_error(E_WARNING, "[Worker %d] Failed to set scheduler policy: %s", worker_id, strerror(errno));
        }
    }
    if (c_options->worker_niceness != 0) {
        if (setpriority(PRIO_PROCESS, 0, c_options->worker_niceness) != 0) {
            php_error(E_WARNING, "[Worker %d] Failed to set niceness: %s", worker_id, strerror(errno));
        }
    }

    /* Set Resource Limits */
    if (c_options->worker_max_open_files > 0) {
        struct rlimit rl = { .rlim_cur = c_options->worker_max_open_files, .rlim_max = c_options->worker_max_open_files };
        if (setrlimit(RLIMIT_NOFILE, &rl) != 0) {
            php_error(E_WARNING, "[Worker %d] Failed to set RLIMIT_NOFILE: %s", worker_id, strerror(errno));
        }
    }

    /* Drop Privileges (change UID/GID) */
    if (c_options->worker_gid > 0) {
        if (setgid(c_options->worker_gid) != 0) {
            php_error(E_ERROR, "[Worker %d] Failed to set GID to %d: %s. Exiting.", worker_id, c_options->worker_gid, strerror(errno));
            exit(1);
        }
    }
    if (c_options->worker_uid > 0) {
        if (setuid(c_options->worker_uid) != 0) {
            php_error(E_ERROR, "[Worker %d] Failed to set UID to %d: %s. Exiting.", worker_id, c_options->worker_uid, strerror(errno));
            exit(1);
        }
    }

    /* Move to cgroup */
    if (c_options->worker_cgroup_path) {
        /* This logic is simplified. Real cgroup v2 interaction is via cgroup.procs file */
        FILE *f = fopen(c_options->worker_cgroup_path, "w");
        if (f) {
            fprintf(f, "%d\n", getpid());
            fclose(f);
        } else {
             php_error(E_WARNING, "[Worker %d] Failed to write to cgroup tasks file '%s': %s", worker_id, c_options->worker_cgroup_path, strerror(errno));
        }
    }

    /* Call the main PHP worker function */
    zval args[1], retval;
    ZVAL_LONG(&args[0], worker_id);
    if (call_user_function_ex(NULL, NULL, &c_options->worker_main_callable, &retval, 1, args, 0, NULL) == FAILURE) {
        php_error(E_ERROR, "[Worker %d] Execution of main worker callable failed.", worker_id);
        exit(1);
    }
    zval_ptr_dtor(&retval);

    /* Worker logic finished, exit cleanly */
    exit(0);
}

/* Signal handler for the master process */
static void cluster_signal_handler(int signo) {
    switch (signo) {
        case SIGINT:
        case SIGTERM:
            g_shutdown_request = 1;
            break;
        case SIGHUP:
            g_reload_request = 1;
            break;
    }
}

/* Helper to parse PHP options array into C struct */
static int parse_options_from_php(zval *php_options_array, quicpro_cluster_options_t *c_options) {
    HashTable *ht = Z_ARRVAL_P(php_options_array);
    zval *zv_temp;

    /* Set defaults */
    c_options->num_workers = sysconf(_SC_NPROCESSORS_ONLN);
    c_options->restart_crashed_workers = 1;
    c_options->max_restarts_per_worker = 5;
    c_options->restart_interval_sec = 60;
    c_options->graceful_shutdown_timeout_sec = 30;
    c_options->worker_loop_usleep_usec = 10000;

    /* REQUIRED: worker_main_callable */
    if (!(zv_temp = zend_hash_str_find(ht, "worker_main_callable", sizeof("worker_main_callable")-1))) {
        throw_mcp_error_as_php_exception(0, "Cluster option 'worker_main_callable' is required.");
        return FAILURE;
    }
    ZVAL_COPY(&c_options->worker_main_callable, zv_temp);
    if (!zend_is_callable(&c_options->worker_main_callable, 0, NULL)) {
        throw_mcp_error_as_php_exception(0, "Cluster option 'worker_main_callable' is not a valid callable.");
        return FAILURE;
    }

    /* Optional settings */
    if ((zv_temp = zend_hash_str_find(ht, "num_workers", sizeof("num_workers")-1)) && Z_TYPE_P(zv_temp) == IS_LONG && Z_LVAL_P(zv_temp) > 0) {
        c_options->num_workers = (int)Z_LVAL_P(zv_temp);
    }

    if ((zv_temp = zend_hash_str_find(ht, "on_worker_start_callable", sizeof("on_worker_start_callable")-1)) && zend_is_callable(zv_temp, 0, NULL)) {
        ZVAL_COPY(&c_options->on_worker_start_callable, zv_temp);
    } else {
        ZVAL_UNDEF(&c_options->on_worker_start_callable);
    }

    if ((zv_temp = zend_hash_str_find(ht, "on_worker_exit_callable", sizeof("on_worker_exit_callable")-1)) && zend_is_callable(zv_temp, 0, NULL)) {
        ZVAL_COPY(&c_options->on_worker_exit_callable, zv_temp);
    } else {
        ZVAL_UNDEF(&c_options->on_worker_exit_callable);
    }

    if ((zv_temp = zend_hash_str_find(ht, "master_pid_file_path", sizeof("master_pid_file_path")-1)) && Z_TYPE_P(zv_temp) == IS_STRING) {
        c_options->master_pid_file_path = estrndup(Z_STRVAL_P(zv_temp), Z_STRLEN_P(zv_temp));
    }

    /* ... TODO: Add parsing for ALL other options from the struct (affinity, niceness, etc.) ... */

    return SUCCESS;
}

/* Helper to free memory allocated during parsing */
static void cleanup_c_options(quicpro_cluster_options_t *c_options) {
    if (c_options->master_pid_file_path) efree(c_options->master_pid_file_path);
    if (c_options->cluster_name) efree(c_options->cluster_name);
    if (c_options->worker_cgroup_path) efree(c_options->worker_cgroup_path);

    if (Z_TYPE(c_options->worker_main_callable) != IS_UNDEF) zval_ptr_dtor(&c_options->worker_main_callable);
    if (Z_TYPE(c_options->on_worker_start_callable) != IS_UNDEF) zval_ptr_dtor(&c_options->on_worker_start_callable);
    if (Z_TYPE(c_options->on_worker_exit_callable) != IS_UNDEF) zval_ptr_dtor(&c_options->on_worker_exit_callable);
}

/* Helpers for PID file management */
static void write_pid_file(const char *path) {
    FILE *f = fopen(path, "w");
    if (f) {
        fprintf(f, "%d", getpid());
        fclose(f);
    }
}
static void remove_pid_file(const char *path) {
    unlink(path);
}