/*
 * cancel.h  –  Error‑to‑PHP‑Exception Translator for php‑quicpro_async
 * --------------------------------------------------------------------
 *
 * The libquiche library signals failures by returning negative integer
 * codes rather than setting errno or throwing exceptions.  In PHP land,
 * however, it is far more idiomatic—and far more user‑friendly—to throw
 * exceptions with meaningful class names and error messages.
 *
 * This header declares the single entry point, quicpro_throw(), which:
 *   1. Accepts the integer error code returned by a quiche API call.
 *   2. Maps that code to one of our Quicpro\Exception subclasses
 *      (e.g., InvalidState, StreamBlocked, TooManyStreams, etc.).
 *   3. Constructs and throws a PHP exception of the appropriate class,
 *      embedding the numeric code and a textual description.
 *   4. Allows PHP userland to catch specific error types via try/catch
 *      blocks instead of parsing raw integers.
 *
 * By centralizing all error‑code translation here, we ensure that every
 * part of the extension reports failures consistently and idiomatically.
 * Include this header in any C source that calls quiche functions whose
 * return values must be turned into PHP exceptions.
 */

#ifndef PHP_QUICPRO_CANCEL_H
#define PHP_QUICPRO_CANCEL_H

#include <php.h>     /* Core PHP API for exception throwing */
#include <quiche.h>  /* libquiche’s definitions of error codes */

/*
 * quicpro_throw()
 *
 * Converts a libquiche error code into the matching PHP exception.
 *
 * Internally, this function examines the provided `quiche_err` integer,
 * selects the correct Quicpro\Exception subclass, and throws it with:
 *   • A descriptive message (e.g., "QUIC stream error 10")
 *   • The original numeric code as the exception’s code property.
 *
 * After calling this helper, control should immediately return
 * from the PHP_FUNCTION implementation (e.g., RETURN_FALSE or RETURN_NULL),
 * since quicpro_throw() does not itself return to the caller.
 *
 * @param quiche_err  The negative integer error code returned by libquiche.
 */
void quicpro_throw(int quiche_err);

#endif /* PHP_QUICPRO_CANCEL_H */
