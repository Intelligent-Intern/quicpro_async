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

help:
	bash infra/scripts/help.sh