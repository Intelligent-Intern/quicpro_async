/*
 * =========================================================================
 * FILENAME: include/quicpro_ini.h
 * MODULE:   quicpro_async: PHP.ini Directive Declarations
 * =========================================================================
 *
 * This header file declares the global variables that store configuration
 * values parsed from php.ini by the Zend Engine. Every other C module in this
 * extension should include this file to access these system-wide defaults.
 *
 * This approach centralizes configuration and decouples modules from the INI
 * parsing mechanism, which is handled exclusively in quicpro_ini.c.
 */

#ifndef PHP_QUICPRO_INI_H
#define PHP_QUICPRO_INI_H

#include <php.h>

/* --- Public helpers (implemented in quicpro_ini.c) --- */

/**
 * @brief Registers all INI directives with the Zend Engine.
 * @param module_number The module number provided during MINIT.
 * @return SUCCESS or FAILURE.
 */
int quicpro_ini_register(int module_number);

/**
 * @brief Unregisters all INI directives during module shutdown.
 * @return SUCCESS or FAILURE.
 */
int quicpro_ini_unregister(void);


/* --------------------------------------------------------------------
 * Exposed read-only globals (populated by Zend from php.ini)
 * ----------------------------------------------------------------- */

/* --- IIBIN (Serialization) & AI Settings --- */
extern zend_long   qp_ini_iibin_max_schema_fields; /* Max fields per schema definition */

/* --- Cluster Supervisor Settings --- */
extern zend_long   qp_ini_cluster_default_workers;  /* Default worker count (0 = auto) */
extern zend_long   qp_ini_cluster_grace_timeout;    /* Graceful shutdown timeout in seconds */

/* --- QUIC / Session / TLS Settings --- */
extern char       *qp_ini_tls_default_ca_file;       /* Default path to CA bundle file */
extern char       *qp_ini_tls_default_cert_file;     /* Default path to server/client cert file */
extern char       *qp_ini_tls_default_key_file;      /* Default path to private key file */
extern zend_long   qp_ini_session_mode;             /* Default session resumption strategy (0=AUTO) */
extern zend_long   qp_ini_session_shm_size;         /* Size of shared memory for ticket ring */
extern char       *qp_ini_session_shm_path;          /* Name of shared memory object */

/* --- General Application Server Defaults --- */
extern zend_long   qp_ini_server_default_port;      /* Default listening port for server applications */
extern char       *qp_ini_server_default_host;       /* Default listening host for server applications */

/* --- Security / Policy Settings --- */
extern zend_bool   qp_ini_allow_config_override;    /* If true, Quicpro\Config objects can override php.ini settings */
extern char       *qp_ini_cors_allowed_origins;      /* Default CORS policy, comma-separated string or '*' */

#endif /* PHP_QUICPRO_INI_H */