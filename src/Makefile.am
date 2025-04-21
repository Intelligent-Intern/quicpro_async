PHP_EXTENSION = quicpro_async
PHP_EXTENSION_SOURCES = php_quicpro.c session.c http3.c poll.c tls.c
PHP_EXTENSION_LDFLAGS = -L$(QUICPRO_ASYNC_LIB) -lquiche -lssl -lcrypto
PHP_EXTENSION_CFLAGS = -I$(QUICPRO_ASYNC_INCLUDE)
include $(top_srcdir)/ext/standard/standard.mk