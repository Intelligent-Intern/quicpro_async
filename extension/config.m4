PHP_ARG_ENABLE([quicpro_async],
  [whether to build the quicpro_async extension],
  [--enable-quicpro_async Build quicpro_async])

if test "$PHP_QUICPRO_ASYNC" != "no"; then
  # Public include dir for our headers
  PHP_ADD_INCLUDE([include])
  # quiche’s own headers
  PHP_ADD_INCLUDE([quiche/include])

  # Link in quiche itself (built under quiche/target/release)
  PHP_ADD_LIBRARY_WITH_PATH([quiche], [quiche/target/release], QUICPRO_ASYNC_SHARED_LIBADD)
  # And OpenSSL
  PHP_ADD_LIBRARY([ssl],   [], QUICPRO_ASYNC_SHARED_LIBADD)
  PHP_ADD_LIBRARY([crypto],[], QUICPRO_ASYNC_SHARED_LIBADD)

  # Make sure the linker flags get picked up
  PHP_SUBST(QUICPRO_ASYNC_SHARED_LIBADD)

  # Finally register our extension and all source files in one go
  PHP_NEW_EXTENSION(quicpro_async, src/cancel.c src/connect.c src/http3.c src/php_quicpro.c src/poll.c src/session.c src/http3.c src/poll.c src/tls.c, $ext_shared)
fi
