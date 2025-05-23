#!/bin/bash

########################################################################################################################
# * # * # * # * # * # * # * # * # * # * # * # * # * # * install docker # * # * # * # * # * # * # * # * # * # * # * # * #
########################################################################################################################

install_docker_and_compose() {
    message "Checking for Docker installation..."
    if [[ -f "$DOCKER_PREFS_FILE" ]]; then
        echo "Docker installation preferences already set. Skipping Docker setup."
        return
    fi
    if ! command -v docker &> /dev/null; then
        if ask_user "Docker is not installed. Do you want to install Docker?"; then
            echo "Installing Docker..."
            sudo apt-get update
            sudo apt-get install -y \
                ca-certificates \
                curl \
                gnupg \
                lsb-release
            echo "Adding Docker GPG key..."
            sudo mkdir -m 0755 -p /etc/apt/keyrings
            # shellcheck disable=SC2046
            curl -fsSL https://download.docker.com/linux/$(. /etc/os-release && echo "$ID")/gpg | sudo gpg --dearmor -o /etc/apt/keyrings/docker.gpg
            echo "Setting up Docker repository..."
            echo \
              "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.gpg] https://download.docker.com/linux/$(. /etc/os-release && echo "$ID") \
              $(lsb_release -cs) stable" | sudo tee /etc/apt/sources.list.d/docker.list > /dev/null
            echo "Installing Docker engine..."
            sudo apt-get update
            sudo apt-get install -y docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin
            echo "Docker installed successfully."
        else
            echo "Skipping Docker installation."
        fi
    else
        echo "Docker is already installed."
        mkdir -p var/preferences
        echo "# This file prevents the Docker installation from being repeated." > "$DOCKER_PREFS_FILE"
    fi
    echo "Checking for Docker Compose installation..."
    if ! docker compose version &> /dev/null; then
        if ask_user "Docker Compose CLI plugin is not installed. Do you want to install Docker Compose?"; then
            echo "Installing Docker Compose..."
            sudo apt-get install -y docker-compose-plugin
            echo "Docker Compose installed successfully."
        else
            echo "Skipping Docker Compose installation."
        fi
    else
        echo "Docker Compose is already installed."
    fi
    echo "# This file prevents the Docker installation from being repeated." > "$DOCKER_PREFS_FILE"
    echo "Docker setup complete. Preferences saved to $DOCKER_PREFS_FILE."
}
