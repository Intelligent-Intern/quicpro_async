/* quicpro_ini.h – Single point of truth for all php.ini directives
 *
 *  + No business logic
 *  + No Zend headers outside this file and quicpro_ini.c
 *  + Every other .c unit calls quicpro_ini_get_* helpers or reads
 *    the exported globals set by Zend after MINIT.
 */
#ifndef PHP_QUICPRO_INI_H
#define PHP_QUICPRO_INI_H

#include <php.h>

/* --------------------------------------------------------------------
 * Public helpers – module init / shutdown must call these.
 * ----------------------------------------------------------------- */
int quicpro_ini_register(int module_number);   /* MINIT  */
int quicpro_ini_unregister(void);              /* MSHUTDOWN */

/* --------------------------------------------------------------------
 * Exposed read-only globals (INI values parsed by Zend)
 * ----------------------------------------------------------------- */
/* Cluster */
extern zend_long   qp_ini_workers;
extern zend_long   qp_ini_port;
extern char       *qp_ini_host;
extern zend_long   qp_ini_usleep_usec;
extern zend_long   qp_ini_grace_timeout;
extern zend_bool   qp_ini_maintenance;
extern zend_long   qp_ini_max_fd;
extern zend_long   qp_ini_max_sessions;
extern zend_bool   qp_ini_metrics_enabled;
extern zend_long   qp_ini_metrics_port;

/* TLS / Config */
extern char       *qp_ini_ca_file;
extern char       *qp_ini_cert_file;
extern char       *qp_ini_key_file;

/* Session Tickets / Shared-Memory ring */
extern zend_long   qp_ini_shm_size;
extern char       *qp_ini_shm_path;
extern zend_long   qp_ini_session_mode;

#endif /* PHP_QUICPRO_INI_H */
