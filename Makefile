.PHONY: build unit fuzz benchmark deploy help

build:
	bash infra/scripts/build.sh

unit:
	bash infra/scripts/unit.sh

fuzz:
	bash infra/scripts/fuzz.sh

benchmark:
	bash infra/scripts/benchmark.sh

deploy:
	bash infra/scripts/deploy.sh

tree:
	bash -c "tree -I 'index.html|rest|infra|tests|get|build|bin|vendor|extension-windows|modules|quiche|test_certs|\\.git|404.html|\\.gitignore|LICENSE|azure-sp\\.json|composer\\.lock|package\\.xml|Makefile\\.am|config\\.h\\.in~|configure~'"

ext-tree:
	bash -c "tree extension/include && echo && echo 'php_quicpro.h:' && ls extension/php_quicpro.h && echo && echo 'src/*.c:' && ls extension/src/cancel.c extension/src/config.c extension/src/connect.c extension/src/http3.c extension/src/php_quicpro.c extension/src/poll.c extension/src/session.c extension/src/tls.c"

infra-tree:
	bash -c "tree infra"


help:
	bash infra/scripts/help.sh