/* quicpro_ini.c – Directive list + MINIT glue
 *
 * All ini entries live here.  Other code includes quicpro_ini.h
 * and reads the exported globals.
 */

#include "quicpro_ini.h"

/* --------------------------------------------------------------------
 * Static backing variables; Zend will write into these.
 * ----------------------------------------------------------------- */
/* clang-format off */
static zend_long  _workers        = 0;
static zend_long  _port           = 4433;
static char      *_host           = NULL;
static zend_long  _usleep_usec    = 0;
static zend_long  _grace_timeout  = 30;
static zend_bool  _maintenance    = 0;
static zend_long  _max_fd         = 8192;
static zend_long  _max_sessions   = 65536;
static zend_bool  _metrics_enabled= 1;
static zend_long  _metrics_port   = 9091;

static char      *_ca_file        = NULL;
static char      *_cert_file      = NULL;
static char      *_key_file       = NULL;

static zend_long  _shm_size       = 131072;
static char      *_shm_path       = NULL;
static zend_long  _session_mode   = 0;

static zend_bool  _allow_config_override = 1;
static char      *_cors_allowed_origins  = NULL;
/* clang-format on */

/* --------------------------------------------------------------------
 * Public aliases exported in header
 * ----------------------------------------------------------------- */
zend_long  qp_ini_workers         = 0;
zend_long  qp_ini_port            = 0;
char      *qp_ini_host            = NULL;
zend_long  qp_ini_usleep_usec     = 0;
zend_long  qp_ini_grace_timeout   = 0;
zend_bool  qp_ini_maintenance     = 0;
zend_long  qp_ini_max_fd          = 0;
zend_long  qp_ini_max_sessions    = 0;
zend_bool  qp_ini_metrics_enabled = 0;
zend_long  qp_ini_metrics_port    = 0;

char      *qp_ini_ca_file         = NULL;
char      *qp_ini_cert_file       = NULL;
char      *qp_ini_key_file        = NULL;

zend_long  qp_ini_shm_size        = 0;
char      *qp_ini_shm_path        = NULL;
zend_long  qp_ini_session_mode    = 0;

zend_bool  qp_ini_allow_config_override = 1;
char      *qp_ini_cors_allowed_origins  = NULL;

/* --------------------------------------------------------------------
 * INI table
 * ----------------------------------------------------------------- */
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("quicpro.workers",               "0",      PHP_INI_SYSTEM, OnUpdateLong,   workers,        zend_long,  NULL)
    STD_PHP_INI_ENTRY("quicpro.port",                  "4433",   PHP_INI_SYSTEM, OnUpdateLong,   port,           zend_long,  NULL)
    STD_PHP_INI_ENTRY("quicpro.host",                  "0.0.0.0",PHP_INI_SYSTEM, OnUpdateString, host,           char *,     NULL)
    STD_PHP_INI_ENTRY("quicpro.usleep_usec",           "0",      PHP_INI_SYSTEM, OnUpdateLong,   usleep_usec,    zend_long,  NULL)
    STD_PHP_INI_ENTRY("quicpro.grace_timeout",         "30",     PHP_INI_SYSTEM, OnUpdateLong,   grace_timeout,  zend_long,  NULL)
    STD_PHP_INI_ENTRY("quicpro.maintenance_mode",      "0",      PHP_INI_SYSTEM, OnUpdateBool,   maintenance,    zend_bool,  NULL)
    STD_PHP_INI_ENTRY("quicpro.max_fd_per_worker",     "8192",   PHP_INI_SYSTEM, OnUpdateLong,   max_fd,         zend_long,  NULL)
    STD_PHP_INI_ENTRY("quicpro.max_sessions",          "65536",  PHP_INI_SYSTEM, OnUpdateLong,   max_sessions,   zend_long,  NULL)
    STD_PHP_INI_ENTRY("quicpro.metrics_enabled",       "1",      PHP_INI_SYSTEM, OnUpdateBool,   metrics_enabled,zend_bool,  NULL)
    STD_PHP_INI_ENTRY("quicpro.metrics_port",          "9091",   PHP_INI_SYSTEM, OnUpdateLong,   metrics_port,   zend_long,  NULL)

    STD_PHP_INI_ENTRY("quicpro.ca_file",               "",       PHP_INI_SYSTEM, OnUpdateString, ca_file,        char *,     NULL)
    STD_PHP_INI_ENTRY("quicpro.client_cert",           "",       PHP_INI_SYSTEM, OnUpdateString, cert_file,      char *,     NULL)
    STD_PHP_INI_ENTRY("quicpro.client_key",            "",       PHP_INI_SYSTEM, OnUpdateString, key_file,       char *,     NULL)

    STD_PHP_INI_ENTRY("quicpro.shm_size",              "131072", PHP_INI_SYSTEM, OnUpdateLong,   shm_size,       zend_long,  NULL)
    STD_PHP_INI_ENTRY("quicpro.shm_path",              "",       PHP_INI_SYSTEM, OnUpdateString, shm_path,       char *,     NULL)
    STD_PHP_INI_ENTRY("quicpro.session_mode",          "0",      PHP_INI_SYSTEM, OnUpdateLong,   session_mode,   zend_long,  NULL)

    STD_PHP_INI_ENTRY("quicpro.allow_config_override", "1",      PHP_INI_SYSTEM, OnUpdateBool, allow_config_override, zend_bool, NULL)
    STD_PHP_INI_ENTRY("quicpro.cors_allowed_origins",  "",       PHP_INI_SYSTEM, OnUpdateString, cors_allowed_origins, char *, NULL)
PHP_INI_END()

/* --------------------------------------------------------------------
 * Helper – copy internal vars to public aliases after INI load
 * ----------------------------------------------------------------- */
static void sync_public_aliases(void)
{
    qp_ini_workers          = _workers;
    qp_ini_port             = _port;
    qp_ini_host             = _host;
    qp_ini_usleep_usec      = _usleep_usec;
    qp_ini_grace_timeout    = _grace_timeout;
    qp_ini_maintenance      = _maintenance;
    qp_ini_max_fd           = _max_fd;
    qp_ini_max_sessions     = _max_sessions;
    qp_ini_metrics_enabled  = _metrics_enabled;
    qp_ini_metrics_port     = _metrics_port;

    qp_ini_ca_file          = _ca_file;
    qp_ini_cert_file        = _cert_file;
    qp_ini_key_file         = _key_file;

    qp_ini_shm_size         = _shm_size;
    qp_ini_shm_path         = _shm_path;
    qp_ini_session_mode     = _session_mode;

    qp_ini_allow_config_override = _allow_config_override;
    qp_ini_cors_allowed_origins  = _cors_allowed_origins;
}

/* --------------------------------------------------------------------
 * Public lifecycle hooks
 * ----------------------------------------------------------------- */
int quicpro_ini_register(int module_number)
{
    REGISTER_INI_ENTRIES();
    sync_public_aliases();
    return SUCCESS;
}

int quicpro_ini_unregister(void)
{
    UNREGISTER_INI_ENTRIES();
    return SUCCESS;
}