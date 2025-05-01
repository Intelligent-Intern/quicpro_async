#!/usr/bin/env bash
set -e

docker-compose --profile php83 up --build --abort-on-container-exit
