/* Error-to-PHP-exception translator
 *
 * libquiche reports failures via negative integer codes rather than C errno.
 * quicpro_throw() maps any libquiche error code into the matching
 * Quicpro\Exception subclass and throws it into the PHP runtime.
 */

#ifndef PHP_QUICPRO_CANCEL_H
#define PHP_QUICPRO_CANCEL_H

#include <php.h>
#include <quiche.h>

/* Throw the PHP exception corresponding to the given libquiche error code. */
void quicpro_throw(int quiche_err);

#endif /* PHP_QUICPRO_CANCEL_H */
