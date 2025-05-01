#!/usr/bin/env bash
set -e

docker-compose --profile php81 up --build --abort-on-container-exit
