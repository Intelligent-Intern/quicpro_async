/*
 * cluster.c – Extended Cluster Supervisor for php-quicpro_async
 * =====================================================================
 * This revision integrates the rich option matrix outlined by the user:
 *   • Dynamic worker-count detection (0 == #CPU cores)
 *   • Host/port tracking for downstream server bootstrap
 *   • Per-IP rate-limiting stub infrastructure (token-bucket)
 *   • Graceful-shutdown drain timer
 *   • Maintenance-mode gate
 *   • Priority matrix / context flags
 *   • Resource limits (RLIMIT_NOFILE)
 *   • Metrics exporter stub (Prometheus-style)
 *   • Health-check ready-file signalling
 *   • Environment + php.ini override helpers (quicpro.* / QUICPRO_*)
 *
 *  NOTE:  This is a *first functional pass* – networking, exporter, and
 *         DoS protection hooks are stubbed; fill in TODOs as the server
 *         codebase grows.  The primary goal is to expose the new options
 *         to PHP, validate input, and propagate the values to master &
 *         worker contexts so that later modules can consume them.
 */

#include "cluster.h"
#include <php.h>
#include <php_ini.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <unistd.h>
#include <sched.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>

/* ────────────────────────────────────────────────────────────────────────
 * Extended Priority / Context bitmasks (match priority_matrix example)
 *────────────────────────────────────────────────────────────────────────*/
#define PRIO_EMERGENCY  0b00000010
#define PRIO_NORMAL     0b00000000
#define PRIO_LOW        0b00000100
#define CTX_API         0b00001000
#define CTX_WEBSOCKET   0b00010000

/* ────────────────────────────────────────────────────────────────────────
 * Rate-limit bucket structure – very small & cache-aligned
 *────────────────────────────────────────────────────────────────────────*/
typedef struct {
    uint32_t allowance;
    uint32_t last_check;   /* unix epoch seconds (coarse) */
    uint8_t  banned;       /* 0/1 flag */
    uint32_t ban_until;    /* epoch seconds */
    uint32_t ip_hash;      /* 32-bit hashed IPv4/IPv6 bucket key */
} rl_bucket_t;

/* ===================================================================== */
/* Globals for master lifecycle                                          */
/* ===================================================================== */
#define MAX_WORKERS 64
static worker_info_t workers[MAX_WORKERS];
static int num_workers = 0;
static int running     = 1;
static quicpro_cluster_opts_t cluster_opts;   /* now hugely extended */

/* Rate-limit table – allocated at runtime based on opts.table_size */
static rl_bucket_t *rl_table = NULL;
static uint32_t     rl_mask  = 0;

/* Forward decls */
static void worker_main(int wid, int cpu);
static void call_php_callback(zval *cb, int wid, int pid, int exit_status);

/* ────────────────────────────────────────────────────────────────────────
 * Utility helpers                                                        */
/*────────────────────────────────────────────────────────────────────────*/
static inline uint32_t next_pow2(uint32_t x)
{
    if (!x) return 1;
    x--; x |= x >> 1; x |= x >> 2; x |= x >> 4; x |= x >> 8; x |= x >> 16;
    return x + 1;
}

static inline uint32_t ip_hash(const char *ip)
{
    /* Very simple FNV-1a 32-bit hash – replace with SipHash if needed */
    uint32_t h = 2166136261u;
    for (const unsigned char *p = (const unsigned char*)ip; *p; p++) {
        h ^= *p;
        h *= 16777619u;
    }
    return h;
}

/* ────────────────────────────────────────────────────────────────────────
 * Rate-limiting stub (token-bucket, coarse 1-sec resolution)
 *────────────────────────────────────────────────────────────────────────*/
static int rl_check_and_consume(const char *ip)
{
    if (!rl_table) return 1; /* disabled */
    uint32_t key  = ip_hash(ip);
    uint32_t idx  = key & rl_mask;
    rl_bucket_t *b = &rl_table[idx];

    uint32_t now = (uint32_t)time(NULL);
    if (!b->ip_hash) {
        /* Unused bucket – claim */
        b->ip_hash   = key;
        b->allowance = cluster_opts.rate_limit.max_per_sec + cluster_opts.rate_limit.burst;
        b->last_check= now;
        b->banned    = 0;
        b->ban_until = 0;
    } else if (b->ip_hash != key) {
        /* Collision – linear probe FNV step (very cheap) */
        /* (Real implementation should do open addressing / cuckoo) */
        idx = (idx + 1) & rl_mask;
        b = &rl_table[idx];
    }

    /* Ban handling */
    if (b->banned) {
        if (now < b->ban_until) return 0; /* still banned */
        b->banned = 0;
        b->allowance = cluster_opts.rate_limit.max_per_sec;
    }

    /* Token refill */
    uint32_t elapsed = now - b->last_check;
    if (elapsed) {
        uint32_t refill = elapsed * cluster_opts.rate_limit.max_per_sec;
        if (b->allowance + refill > cluster_opts.rate_limit.max_per_sec + cluster_opts.rate_limit.burst)
            b->allowance = cluster_opts.rate_limit.max_per_sec + cluster_opts.rate_limit.burst;
        else
            b->allowance += refill;
        b->last_check = now;
    }

    if (b->allowance) {
        b->allowance--; /* consume */
        return 1;
    }

    /* Exceeded – ban & maybe log */
    b->banned    = 1;
    b->ban_until = now + cluster_opts.rate_limit.ban_seconds;
    if (cluster_opts.rate_limit.log_drops) {
        php_error(E_NOTICE, "[quicpro] IP %s temporarily banned for rate-limit", ip);
    }
    return 0;
}

/* ────────────────────────────────────────────────────────────────────────
 * Graceful shutdown timer & signal management                             */
/*────────────────────────────────────────────────────────────────────────*/
static void handle_sigterm(int signo)
{
    running = 0;
}

static void setup_signal_handlers(void)
{
    struct sigaction sa = {0};
    sa.sa_handler = handle_sigterm;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGHUP,  &sa, NULL);
    sa.sa_handler = SIG_DFL;
    sigaction(SIGCHLD, &sa, NULL);
}

/* ────────────────────────────────────────────────────────────────────────
 * RLimit helper                                                           */
/*────────────────────────────────────────────────────────────────────────*/
static void apply_fd_limit(rlim_t max_fd)
{
    struct rlimit rl = { max_fd, max_fd };
    if (setrlimit(RLIMIT_NOFILE, &rl) != 0) {
        php_error(E_WARNING, "[quicpro] setrlimit(RLIMIT_NOFILE) failed: %s", strerror(errno));
    }
}

/* ===================================================================== */
/* Worker code                                                            */
/* ===================================================================== */
static void worker_main(int wid, int cpu)
{
    if (cluster_opts.affinity) set_affinity(cpu);
    set_priority(cluster_opts.priority_flags);

    /* Resource limits */
    if (cluster_opts.max_fd_per_worker > 0)
        apply_fd_limit(cluster_opts.max_fd_per_worker);

    /* Ready-file: clear then touch later */
    if (wid == 0 && cluster_opts.ready_file[0]) {
        unlink(cluster_opts.ready_file);
    }

    /* User callback */
    call_php_callback(&cluster_opts.on_start_cb, wid, getpid(), 0);

    /* TODO: open QUIC listening socket on cluster_opts.port/host.  This
     * will be handled by higher-level server code; for now just loop. */

    /* Maintenance mode gate */
    if (cluster_opts.maintenance_mode) {
        /* Only allow health-check path; others rejected downstream */
    }

    /* Busyloop / event loop */
    while (running) {
        /* TODO: Accept+poll server events, apply rl_check_and_consume() on each new req */

        if (cluster_opts.usleep_usec)
            usleep(cluster_opts.usleep_usec);
        else
            sched_yield();
    }

    /* Graceful shutdown drain */
    time_t deadline = time(NULL) + cluster_opts.graceful_shutdown_timeout;
    while (time(NULL) < deadline) {
        /* TODO: Drive event loop until sessions drained */
        usleep(10000);
    }

    /* Write ready_file removal to signal worker gone */
    if (wid == 0 && cluster_opts.ready_file[0])
        unlink(cluster_opts.ready_file);

    _exit(0);
}

/* ===================================================================== */
/* Master process logic (spawn & supervise workers)                       */
/* ===================================================================== */
static void spawn_workers(void)
{
    int cpu = 0;
    for (int i = 0; i < cluster_opts.num_workers; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            worker_main(i, cpu);
            _exit(0);
        }
        if (pid < 0) {
            php_error(E_ERROR, "[quicpro] fork() failed: %s", strerror(errno));
            exit(1);
        }
        workers[i] = (worker_info_t){ .pid = pid, .wid = i, .cpu = cpu };
        cpu = (cpu + 1) % sysconf(_SC_NPROCESSORS_ONLN);
    }
    num_workers = cluster_opts.num_workers;
}

static void supervisor_loop(void)
{
    setup_signal_handlers();

    /* Ready-file: master writes once cluster is fully booted */
    if (cluster_opts.ready_file[0]) {
        int fd = open(cluster_opts.ready_file, O_CREAT|O_WRONLY, 0644);
        if (fd >= 0) close(fd);
    }

    while (running) {
        int status;
        pid_t pid = waitpid(-1, &status, WNOHANG);
        if (pid > 0) {
            int wid = -1;
            for (int i = 0; i < num_workers; i++) if (workers[i].pid == pid) { wid = workers[i].wid; break; }
            if (wid >= 0) {
                call_php_callback(&cluster_opts.on_crash_cb, wid, pid, status);
                /* Restart */
                pid_t npid = fork();
                if (npid == 0) {
                    worker_main(wid, workers[wid].cpu);
                    _exit(0);
                }
                workers[wid].pid = npid;
            }
        }
        /* TODO: metrics exporter heartbeat, maintenance mode reload */
        usleep(5000);
    }

    /* Terminate workers */
    for (int i = 0; i < num_workers; i++) {
        kill(workers[i].pid, SIGTERM);
    }
    for (int i = 0; i < num_workers; i++) {
        waitpid(workers[i].pid, NULL, 0);
    }
}

/* ===================================================================== */
/* Option parsing helpers – from PHP array + env + ini                    */
/* ===================================================================== */
static inline long ini_or_default(const char *name, long def)
{
    zend_long v;
    if (cfg_get_long(name, &v) == SUCCESS) return (long)v;
    return def;
}

static inline long env_or_default(const char *name, long def)
{
    const char *e = getenv(name);
    return e ? strtol(e, NULL, 10) : def;
}

static void parse_cluster_options(zval *zopts)
{
    memset(&cluster_opts, 0, sizeof(cluster_opts));

    /* === scalar opts =================================================== */
    cluster_opts.num_workers  = (int)ini_or_default("quicpro.workers", 0);
    cluster_opts.port         = (int)ini_or_default("quicpro.port", 4433);
    cluster_opts.usleep_usec  = (int)ini_or_default("quicpro.usleep_usec", 0);
    cluster_opts.graceful_shutdown_timeout = (int)ini_or_default("quicpro.graceful_shutdown_timeout", 30);
    cluster_opts.maintenance_mode = ini_or_default("quicpro.maintenance_mode", 0);
    cluster_opts.max_fd_per_worker= ini_or_default("quicpro.max_fd_per_worker", 8192);
    cluster_opts.max_sessions     = ini_or_default("quicpro.max_sessions", 65536);
    cluster_opts.metrics_enabled  = ini_or_default("quicpro.metrics_enabled", 1);
    cluster_opts.metrics_port     = ini_or_default("quicpro.metrics_port", 9091);

    /* Host env override */
    const char *host_env = getenv("QUICPRO_HOST");
    if (host_env) strncpy(cluster_opts.host, host_env, sizeof(cluster_opts.host)-1);
    else strcpy(cluster_opts.host, "0.0.0.0");

    /* Ready-file default */
    strcpy(cluster_opts.ready_file, "/tmp/quicpro.ready");

    /* === array opts from PHP =========================================== */
    if (!zopts || Z_TYPE_P(zopts) != IS_ARRAY) return;
    HashTable *ht = Z_ARRVAL_P(zopts);
    zval *z;

#define GET_LONG(field,name) if ((z=zend_hash_str_find(ht,name,sizeof(name)-1)) && Z_TYPE_P(z)==IS_LONG) cluster_opts.field = Z_LVAL_P(z);
#define GET_BOOL(field,name) if ((z=zend_hash_str_find(ht,name,sizeof(name)-1))) cluster_opts.field = zend_is_true(z);
#define GET_STRBUF(buf,name) if ((z=zend_hash_str_find(ht,name,sizeof(name)-1)) && Z_TYPE_P(z)==IS_STRING) strncpy(buf, Z_STRVAL_P(z), sizeof(buf)-1);

    GET_LONG(num_workers, "workers");
    GET_LONG(port,        "port");
    GET_STRBUF(host,      "host");
    GET_LONG(usleep_usec, "usleep_usec");
    GET_BOOL(maintenance_mode, "maintenance_mode");
    GET_LONG(graceful_shutdown_timeout, "graceful_shutdown_timeout");
    GET_LONG(max_fd_per_worker, "max_fd_per_worker");
    GET_LONG(max_sessions, "max_sessions");
    GET_BOOL(metrics_enabled, "metrics_enabled");
    GET_LONG(metrics_port, "metrics_port");
    GET_STRBUF(ready_file, "ready_file");

    /* Rate-limit sub-array */
    if ((z = zend_hash_str_find(ht, "rate_limit", sizeof("rate_limit")-1)) && Z_TYPE_P(z)==IS_ARRAY) {
        HashTable *rh = Z_ARRVAL_P(z);
        GET_LONG(rate_limit.max_per_sec, "max_per_sec");
        GET_LONG(rate_limit.burst,       "burst");
        GET_LONG(rate_limit.ban_seconds, "ban_seconds");
        GET_LONG(rate_limit.table_size,  "table_size");
        GET_BOOL(rate_limit.log_drops,   "log_drops");
    }
#undef GET_LONG
#undef GET_BOOL
#undef GET_STRBUF

    /* Auto workers (#cores) */
    if (cluster_opts.num_workers <= 0)
        cluster_opts.num_workers = (int)sysconf(_SC_NPROCESSORS_ONLN);
    if (cluster_opts.num_workers > MAX_WORKERS)
        cluster_opts.num_workers = MAX_WORKERS;

    /* Allocate rate-limit table power-of-two */
    if (cluster_opts.rate_limit.table_size < 16) cluster_opts.rate_limit.table_size = 1024;
    uint32_t sz = next_pow2(cluster_opts.rate_limit.table_size);
    rl_table = ecalloc(sz, sizeof(rl_bucket_t));
    rl_mask  = sz - 1;
}

/* ===================================================================== */
/* PHP entrypoint                                                         */
/* ===================================================================== */
PHP_FUNCTION(quicpro_cluster_spawn_workers)
{
    zval *zopts = NULL;
    ZEND_PARSE_PARAMETERS_START(0,1)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY(zopts)
    ZEND_PARSE_PARAMETERS_END();

    parse_cluster_options(zopts);

    if (getpid() == 1) {
        php_error(E_ERROR, "[quicpro] Do not run as PID 1");
        RETURN_FALSE;
    }

    spawn_workers();
    supervisor_loop();

    RETURN_TRUE;
}
