#!/usr/bin/env bash
set -e

docker-compose --profile php82 up --build --abort-on-container-exit
