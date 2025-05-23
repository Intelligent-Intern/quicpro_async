#!/bin/bash
set -e

if [[ "$(whoami)" != "root" ]]; then
  echo "This script must be run as root to stop processes and modify var folder."
  exit 0;
fi

# change to project root
cd "$DIR"/../../ || exit
# Path to docker-compose.yml - because if we are inside project root then there should be this file
DOCKER_COMPOSE_FILE="./docker-compose.yml"
# Check if utils.sh exists
if [[ ! -f "$DOCKER_COMPOSE_FILE" ]]; then
    echo "Error: Required file '$DOCKER_COMPOSE_FILE' does not exist - we might not be in the right directory. Exiting."
    exit 1
fi

source ./infra/scripts/utils.sh
source ./infra/scripts/container-utils.sh
source ./infra/scripts/install-docker.sh

LOCAL_USER="${SUDO_USER:-$(whoami)}"
LOCAL_GROUP=$(id -gn "$LOCAL_USER")
if ! getent group "$(id -g "$LOCAL_USER")" &> /dev/null; then

    echo "Error: Required file '$DOCKER_COMPOSE_FILE' does not exist - we might not be in the right directory. Exiting."
    exit 1

    echo "Group with GID $(id -g "$LOCAL_USER") not found. Creating group..."
    sudo groupadd --gid "$(id -g "$LOCAL_USER")" "$LOCAL_GROUP"
fi

