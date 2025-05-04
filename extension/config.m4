PHP_ARG_ENABLE([quicpro_async],
  [whether to build the quicpro_async extension],
  [--enable-quicpro_async Build quicpro_async])

if test "$PHP_QUICPRO_ASYNC" != "no"; then
  PHP_ADD_INCLUDE([quiche/include])
  PHP_ADD_LIBRARY_WITH_PATH([quiche], [quiche/target/release], QUICPRO_ASYNC_SHARED_LIBADD)
  PHP_ADD_LIBRARY([ssl], [], QUICPRO_ASYNC_SHARED_LIBADD)
  PHP_ADD_LIBRARY([crypto], [], QUICPRO_ASYNC_SHARED_LIBADD)
  PHP_SUBST(QUICPRO_ASYNC_SHARED_LIBADD)

  PHP_NEW_EXTENSION(quicpro_async, src/php_quicpro.c src/session.c src/http3.c src/poll.c src/tls.c src/cancel.c, $ext_shared)
fi
