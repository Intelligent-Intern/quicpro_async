#!/usr/bin/env bash
set -e

docker-compose --profile php84 up --build --abort-on-container-exit
