/*
 * connect.h – Public interface for QUIC connection setup in php-quicpro_async
 * --------------------------------------------------------------------------
 * This header declares the function prototype for quicpro_connect(),
 * the main entry point for establishing a client-side QUIC/HTTP3 session
 * in the extension. It is included by connect.c and referenced by the
 * extension function registration logic. No implementation details or
 * private structures are present here—only userland-callable APIs
 * or helpers required for linking.
 *
 * Note: Any shared helper macros or static inlines *only* used by
 * connect.c should remain in the .c file. This header is intentionally
 * minimal and avoids redundant declarations found in php_quicpro.h.
 */

#ifndef QUICPRO_CONNECT_H
#define QUICPRO_CONNECT_H

/* Prototype for the PHP userland connection function */
PHP_FUNCTION(quicpro_connect);

#endif /* QUICPRO_CONNECT_H */
