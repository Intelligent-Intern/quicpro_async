/* cluster_opts.h – Centralised option model and sane defaults
 *
 * Contains:
 *   • Public typedefs for all cluster-level settings
 *   • Compile-time defaults
 *   • Function prototypes for parsing ini, env and PHP arrays
 *
 * Do NOT include fork(2), signal(2) or heavy platform headers here;
 * keep this header pure so it can be reused by unit tests.
 */
#ifndef QUICPRO_CLUSTER_OPTS_H
#define QUICPRO_CLUSTER_OPTS_H

#include <php.h>
#include <stdint.h>

/* -------------------------------------------------------------------------
 * Default values (can be overridden via php.ini, environment, or PHP array)
 * ---------------------------------------------------------------------- */
#define QP_CL_DEFAULT_WORKERS                 0           /* auto = CPU cores   */
#define QP_CL_DEFAULT_PORT                    4433
#define QP_CL_DEFAULT_HOST                    "0.0.0.0"
#define QP_CL_DEFAULT_USLEEP_USEC             0
#define QP_CL_DEFAULT_GRACE_TIMEOUT_SEC       30
#define QP_CL_DEFAULT_MAINTENANCE_MODE        0
#define QP_CL_DEFAULT_MAX_FD_PER_WORKER       8192
#define QP_CL_DEFAULT_MAX_SESSIONS            65536
#define QP_CL_DEFAULT_METRICS_ENABLED         1
#define QP_CL_DEFAULT_METRICS_PORT            9091
#define QP_CL_DEFAULT_LOG_ENABLED             1
#define QP_CL_DEFAULT_LOG_DIR                 "/var/log/quicpro"
#define QP_CL_DEFAULT_ACCESS_LOG_FMT          "json"
#define QP_CL_DEFAULT_HEALTH_PATH             "/.hc"
#define QP_CL_DEFAULT_READY_FILE              "/tmp/quicpro.ready"
#define QP_CL_DEFAULT_SERVICE_MESH_ENABLED    0
#define QP_CL_DEFAULT_SERVICE_MESH_PORT       7070

/* -------------------------------------------------------------------------
 * Rate-limit defaults
 * ---------------------------------------------------------------------- */
#define QP_RL_DEFAULT_MAX_PER_SEC             100
#define QP_RL_DEFAULT_BURST                   20
#define QP_RL_DEFAULT_BAN_SECONDS             10
#define QP_RL_DEFAULT_TABLE_SIZE              4096
#define QP_RL_DEFAULT_LOG_DROPS               1

/* -------------------------------------------------------------------------
 * Priority/Context bit masks (shared with cluster.h)
 * ---------------------------------------------------------------------- */
enum {
    QP_PRIO_EMERGENCY = 0b00000001,
    QP_PRIO_HIGH      = 0b00000010,
    QP_PRIO_NORMAL    = 0b00000100,
    QP_PRIO_LOW       = 0b00001000,

    QP_MODE_REALTIME  = 0b00010000,

    QP_CTX_API        = 0b00100000,
    QP_CTX_WEBSOCKET  = 0b01000000
};

/* -------------------------------------------------------------------------
 * Nested option groups
 * ---------------------------------------------------------------------- */
typedef struct {
    uint32_t max_per_sec;
    uint32_t burst;
    uint32_t ban_seconds;
    uint32_t table_size;
    zend_bool log_drops;
} qp_rate_limit_opts_t;

typedef struct {
    uint32_t critical_control;
    uint32_t normal_api;
    uint32_t low_ws;
} qp_priority_matrix_t;

/* -------------------------------------------------------------------------
 * Main cluster option struct
 * ---------------------------------------------------------------------- */
typedef struct {
    uint32_t workers;
    uint16_t port;
    char     host[64];

    uint32_t usleep_usec;

    qp_rate_limit_opts_t  rate;
    uint32_t graceful_shutdown_timeout;
    zend_bool maintenance_mode;
    qp_priority_matrix_t  priority;

    uint32_t max_fd_per_worker;
    uint32_t max_sessions;

    zend_bool metrics_enabled;
    uint16_t  metrics_port;

    zend_bool log_enabled;
    char      log_dir[256];
    char      access_log_format[16];

    char      health_check_path[32];
    char      ready_file[128];

    zend_bool service_mesh_enabled;
    uint16_t  service_mesh_port;

    zval      on_worker_start;
    zval      on_session_open;
    zval      on_session_close;
} qp_cluster_opts_t;

/* -------------------------------------------------------------------------
 * API
 * ---------------------------------------------------------------------- */
/* Fill struct with compile-time defaults */
void qp_cluster_opts_init(qp_cluster_opts_t *dst);

/* Apply php.ini entries (module globals), return 0 on success */
int  qp_cluster_opts_apply_ini(qp_cluster_opts_t *dst);

/* Apply environment variables (QUICPRO_*), return number applied */
int  qp_cluster_opts_apply_env(qp_cluster_opts_t *dst);

/* Merge options from a PHP array (userland), honour types & bounds */
int  qp_cluster_opts_apply_hash(qp_cluster_opts_t *dst, HashTable *ht);

/* Destroy zvals in callbacks to avoid leaks */
void qp_cluster_opts_dtor(qp_cluster_opts_t *dst);

#endif /* QUICPRO_CLUSTER_OPTS_H */
