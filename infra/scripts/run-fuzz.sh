#!/usr/bin/env bash

# run with "make fuzz" from package root (../../)
set -e
cd "$(dirname "${BASH_SOURCE[0]}")/../../"
cd tests/fuzz
make
