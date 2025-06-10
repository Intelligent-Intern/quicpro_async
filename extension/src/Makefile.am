PHP_EXTENSION = quicpro_async

PHP_EXTENSION_SOURCES = \
    cancel.c \
    cluster.c \
    config.c \
    connect.c \
    http3.c \
    iibin.c \
    iibin_decoding.c \
    iibin_encoding.c \
    iibin_schema.c \
    mcp.c \
    php_quicpro.c \
    pipeline_orchestrator.c \
    poll.c \
    quicpro_ini.c \
    session.c \
    tls.c \
    tool_handler_registry.c \
    websocket.c

PHP_EXTENSION_LDFLAGS = -L$(QUICPRO_ASYNC_LIB) -lquiche -lssl -lcrypto
PHP_EXTENSION_CFLAGS = -I$(QUICPRO_ASYNC_INCLUDE)

include $(top_srcdir)/ext/standard/standard.mk