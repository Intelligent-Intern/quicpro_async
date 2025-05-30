/*
 * cancel.h – Exception mapping, QUIC stream shutdown, and core error helpers
 * for php-quicpro_async.
 *
 * This header defines core error handling mechanisms, converting native C errors
 * (from libquiche or internal modules like Proto, MCP, PipelineOrchestrator)
 * into PHP exceptions. It also provides helpers for QUIC stream shutdown.
 *
 * All implementation details are provided in cancel.c.
 */

#ifndef QUICPRO_CANCEL_H
#define QUICPRO_CANCEL_H

#include <php.h>
/* Include quiche.h if QUICHE_SHUTDOWN_READ/WRITE are used in this header,
 * or ensure they are defined elsewhere if how_to_flags is kept public here.
 * For libquiche error codes, cancel.c will include quiche.h.
 */
// #include <quiche.h>


/*
 * Error Domains (Conceptual - for organizing error handling)
 * These might be C enums or defines used internally if you create a more
 * generic error throwing function.
 */
// typedef enum {
//     QUICPRO_ERROR_DOMAIN_QUICHE,
//     QUICPRO_ERROR_DOMAIN_PROTO,
//     QUICPRO_ERROR_DOMAIN_MCP,
//     QUICPRO_ERROR_DOMAIN_PIPELINE,
//     /* ... other domains ... */
// } quicpro_error_domain_t;


/*
 * how_to_flags
 * ------------
 * C Utility: Converts a shutdown mode string ("read", "write", "both")
 * into the corresponding bitmask for libquiche's quiche_conn_stream_shutdown.
 * (Declaration remains useful if other C modules need this)
 */
int how_to_flags(const char *how, size_t len);

/*
 * throw_quiche_error_as_php_exception
 * -----------------------------------
 * C Utility: Maps a negative libquiche error code to the corresponding PHP exception class
 * (e.g., Quicpro\Exception\StreamLimitException).
 * Throws the PHP exception.
 *
 * Parameters:
 * - quiche_err: Negative integer error code from libquiche.
 * - default_message_format: Optional printf-style format string for a default message if no specific mapping exists.
 */
void throw_quiche_error_as_php_exception(int quiche_err, const char *default_message_format, ...);

/*
 * throw_mcp_error_as_php_exception
 * --------------------------------
 * C Utility: Maps an MCP module-specific error code to a PHP Quicpro\Exception\MCPException
 * or one of its subclasses. Throws the PHP exception.
 *
 * Parameters:
 * - mcp_err_code: Error code specific to the MCP C module.
 * - message_format: Printf-style format string for the exception message.
 */
void throw_mcp_error_as_php_exception(int mcp_err_code, const char *message_format, ...);

/*
 * throw_proto_error_as_php_exception
 * ----------------------------------
 * C Utility: Maps a Proto module-specific error code to a PHP Quicpro\Exception\ProtoException
 * or one of its subclasses. Throws the PHP exception.
 *
 * Parameters:
 * - proto_err_code: Error code specific to the Proto C module.
 * - message_format: Printf-style format string for the exception message.
 */
void throw_proto_error_as_php_exception(int proto_err_code, const char *message_format, ...);

/*
 * throw_pipeline_error_as_php_exception
 * -------------------------------------
 * C Utility: Maps a PipelineOrchestrator module-specific error code to a PHP
 * Quicpro\Exception\PipelineException or one of its subclasses. Throws the PHP exception.
 *
 * Parameters:
 * - pipeline_err_code: Error code specific to the Pipeline C module.
 * - message_format: Printf-style format string for the exception message.
 */
void throw_pipeline_error_as_php_exception(int pipeline_err_code, const char *message_format, ...);


/* --- PHP Userland Functions related to Cancellation & Errors --- */

/*
 * PHP_FUNCTION(quicpro_cancel_stream);
 * ------------------------------------
 * PHP Userland API: Performs a half-close or full-close on a QUIC stream
 * associated with an active MCP connection resource.
 *
 * Userland Signature (from php_quicpro_arginfo.h):
 * bool quicpro_cancel_stream(resource $mcp_connection, int $stream_id, string $how = "both")
 */
PHP_FUNCTION(quicpro_cancel_stream);

/*
 * PHP_FUNCTION(quicpro_get_last_general_error);
 * ---------------------------------------------
 * (If you maintain a general last error string for non-exception cases, or for debugging)
 * Retrieves the last non-exception error message set by the extension.
 * This is often superseded by throwing specific exceptions but can be a fallback.
 *
 * Userland Signature (from php_quicpro_arginfo.h):
 * string quicpro_get_last_general_error(void)
 *
 * (Note: Your php_quicpro.h already declares quicpro_get_last_error, this might be the same or refined)
 */
// PHP_FUNCTION(quicpro_get_last_general_error); // If distinct from quicpro_mcp_get_error etc.


#endif /* QUICPRO_CANCEL_H */