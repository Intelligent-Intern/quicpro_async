PHP_EXTENSION = quicpro_async
PHP_EXTENSION_SOURCES = cancel.c connect.c http3.c php_quicpro.c poll.c session.c http3.c tls.c
PHP_EXTENSION_LDFLAGS = -L$(QUICPRO_ASYNC_LIB) -lquiche -lssl -lcrypto
PHP_EXTENSION_CFLAGS = -I$(QUICPRO_ASYNC_INCLUDE)
include $(top_srcdir)/ext/standard/standard.mk