/* include/tls.h  ── TLS-Optionen & Session-Ticket APIs für php-quicpro_async */

#ifndef PHP_QUICPRO_TLS_H
#define PHP_QUICPRO_TLS_H

#include "php_quicpro.h"

/* -------------------------------------------------------------------------
 * PHP-Funktionsprototypen
 * ------------------------------------------------------------------------- */

/* Setzt die CA-Zertifikatsdatei für alle künftigen Sessions */
PHP_FUNCTION(quicpro_set_ca_file);

/* Setzt Client-Zertifikat und privaten Schlüssel für mTLS */
PHP_FUNCTION(quicpro_set_client_cert);

/* Exportiert das Session-Ticket als String (QUICPRO_MAX_TICKET_SIZE bytes) */
PHP_FUNCTION(quicpro_export_session_ticket);

/* Importiert ein zuvor exportiertes Session-Ticket */
PHP_FUNCTION(quicpro_import_session_ticket);

#endif /* PHP_QUICPRO_TLS_H */