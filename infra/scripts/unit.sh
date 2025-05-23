#!/usr/bin/env bash

set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/../../"

show_bar() {
    local message=$1
    local percent=$2
    local width=50
    local filled=$((percent * width / 100))
    local empty=$((width - filled))

    export LANG=en_US.UTF-8
    export LC_ALL=en_US.UTF-8

    local term_width=$(tput cols 2>/dev/null || echo 80)
    local pad=$(( (term_width - width - 10 - ${#message}) / 2 ))

    printf "%*s" "$pad" ""

    printf "["
    printf "\033[47m%*s\033[0m" "$filled" | tr ' ' ' '
    printf "\033[100m%*s\033[0m" "$empty" | tr ' ' ' '
    printf "] %3d%% - %s\n" "$percent" "$message"

    sleep 0.1
}


log_and_fail() {
    local container=$1
    local message=$2
    local output=$3

    clear
    echo -e "\e[41m${message}\e[0m"
    echo "$output"
    echo -e "\n\e[33m=== Container Logs ($container) ===\e[0m"
    docker logs "$container" || true
    exit 1
}

DOCKER_COMPOSE_FILE="./docker-compose.yml"
if [[ ! -f "$DOCKER_COMPOSE_FILE" ]]; then
    echo "Error: Required file '$DOCKER_COMPOSE_FILE' is missing. Are you in the right directory?"
    exit 1
fi

source ./infra/scripts/utils.sh || true
source ./infra/scripts/container-utils.sh || true

clear
show_bar "Checking containers..." 0
clear
ensure_containers
clear
check_and_install_mkcert
clear
update_certs
clear
update_hosts
clear
docker ps -a --filter name=php81-test --filter name=php82-test --filter name=php83-test --filter name=php84-test --filter name=quic-demo -q | xargs -r docker rm -f 2>&1

clear
show_bar "Starting infra containers..." 3
docker compose --profile infra up --build --force-recreate -d > /dev/null 2>&1

clear
show_bar "Starting test containers..." 6
docker compose --profile test up --build --force-recreate -d > /dev/null 2>&1

clear
show_bar "quic-demo: Building quiche..." 9
output=$(docker exec -e TERM=dumb quic-demo sh -c "cd /workspace/quiche && cargo build --release" 2>&1) || log_and_fail quic-demo "❌ Failed to build quiche" "$output"

clear
show_bar "quic-demo: Building extension..." 12
output=$(docker exec -it quic-demo sh -c "cd /workspace && make -s build" 2>&1)
echo "$output" | grep -q "Build complete" || log_and_fail quic-demo "❌ Failed to build extension" "$output"

clear
show_bar "quic-demo: Running composer install..." 15
output=$(docker exec -e TERM=dumb quic-demo sh -c "cd /workspace && composer install" 2>&1) || log_and_fail quic-demo "❌ Composer install failed" "$output"

clear
show_bar "quic-demo: Writing INI..." 18
docker exec -e TERM=dumb quic-demo sh -c "echo 'extension=quicpro_async.so' > /etc/php/8.4/cli/conf.d/99-quicpro_async.ini" > /dev/null 2>&1

clear
show_bar "quic-demo: Running smoke test..." 21
output=$(docker exec -it quic-demo sh -c "php /workspace/tests/utils/run-tests.php /workspace/tests/phpunit/001-smoke/test.phpt" 2>&1)
echo "$output" | grep -q "Tests passed" || log_and_fail quic-demo "❌ Smoke test failed" "$output"

clear
show_bar "php84-test: Building quiche..." 24
output=$(docker exec -e TERM=dumb php84-test sh -c "cd /workspace/quiche && cargo build --release" 2>&1) || log_and_fail php84-test "❌ Failed to build quiche (php84-test)" "$output"

clear
show_bar "php84-test: Building extension..." 27
output=$(docker exec -it php84-test sh -c "cd /workspace && make -s build" 2>&1)
echo "$output" | grep -q "Build complete" || log_and_fail php84-test "❌ Failed to build extension (php84-test)" "$output"

clear
show_bar "php84-test: Running composer install..." 30
output=$(docker exec -e TERM=dumb php84-test sh -c "cd /workspace && composer install" 2>&1) || log_and_fail php84-test "❌ Composer install failed (php84-test)" "$output"

clear
show_bar "php84-test: Writing INI..." 33
docker exec -e TERM=dumb php84-test sh -c "echo 'extension=quicpro_async.so' > /etc/php/8.4/cli/conf.d/99-quicpro_async.ini" > /dev/null 2>&1

clear
show_bar "php84-test: Running smoke test..." 36
output=$(docker exec -it php84-test sh -c "php /workspace/tests/utils/run-tests.php /workspace/tests/phpunit/001-smoke/test.phpt" 2>&1)
echo "$output" | grep -q "Tests passed" || log_and_fail php84-test "❌ Smoke test failed (php84-test)" "$output"

clear
show_bar "php83-test: Building quiche..." 39
output=$(docker exec -e TERM=dumb php83-test sh -c "cd /workspace/quiche && cargo build --release" 2>&1) || log_and_fail php83-test "❌ Failed to build quiche (php83-test)" "$output"

clear
show_bar "php83-test: Building extension..." 42
output=$(docker exec -it php83-test sh -c "cd /workspace && make -s build" 2>&1)
echo "$output" | grep -q "Build complete" || log_and_fail php83-test "❌ Failed to build extension (php83-test)" "$output"

clear
show_bar "php83-test: Running composer install..." 45
output=$(docker exec -e TERM=dumb php83-test sh -c "cd /workspace && composer install" 2>&1) || log_and_fail php83-test "❌ Composer install failed (php83-test)" "$output"

clear
show_bar "php83-test: Writing INI..." 48
docker exec -e TERM=dumb php83-test sh -c "echo 'extension=quicpro_async.so' > /etc/php/8.3/cli/conf.d/99-quicpro_async.ini" > /dev/null 2>&1

clear
show_bar "php83-test: Running smoke test..." 51
output=$(docker exec -it php83-test sh -c "php /workspace/tests/utils/run-tests.php /workspace/tests/phpunit/001-smoke/test.phpt" 2>&1)
echo "$output" | grep -q "Tests passed" || log_and_fail php83-test "❌ Smoke test failed (php83-test)" "$output"

clear
show_bar "php82-test: Building quiche..." 54
output=$(docker exec -e TERM=dumb php82-test sh -c "cd /workspace/quiche && cargo build --release" 2>&1) || log_and_fail php82-test "❌ Failed to build quiche (php82-test)" "$output"

clear
show_bar "php82-test: Building extension..." 57
output=$(docker exec -it php82-test sh -c "cd /workspace && make -s build" 2>&1)
echo "$output" | grep -q "Build complete" || log_and_fail php82-test "❌ Failed to build extension (php82-test)" "$output"

clear
show_bar "php82-test: Running composer install..." 60
output=$(docker exec -e TERM=dumb php82-test sh -c "cd /workspace && composer install" 2>&1) || log_and_fail php82-test "❌ Composer install failed (php82-test)" "$output"

clear
show_bar "php82-test: Writing INI..." 63
docker exec -e TERM=dumb php82-test sh -c "echo 'extension=quicpro_async.so' > /etc/php/8.2/cli/conf.d/99-quicpro_async.ini" > /dev/null 2>&1

clear
show_bar "php82-test: Running smoke test..." 66
output=$(docker exec -it php82-test sh -c "php /workspace/tests/utils/run-tests.php /workspace/tests/phpunit/001-smoke/test.phpt" 2>&1)
echo "$output" | grep -q "Tests passed" || log_and_fail php82-test "❌ Smoke test failed (php82-test)" "$output"

clear
show_bar "php81-test: Building quiche..." 69
output=$(docker exec -e TERM=dumb php81-test sh -c "cd /workspace/quiche && cargo build --release" 2>&1) || log_and_fail php81-test "❌ Failed to build quiche (php81-test)" "$output"

clear
show_bar "php81-test: Building extension..." 72
output=$(docker exec -it php81-test sh -c "cd /workspace && make -s build" 2>&1)
echo "$output" | grep -q "Build complete" || log_and_fail php81-test "❌ Failed to build extension (php81-test)" "$output"

clear
show_bar "php81-test: Running composer install..." 75
output=$(docker exec -e TERM=dumb php81-test sh -c "cd /workspace && composer install" 2>&1) || log_and_fail php81-test "❌ Composer install failed (php81-test)" "$output"

clear
show_bar "php81-test: Writing INI..." 78
docker exec -e TERM=dumb php81-test sh -c "echo 'extension=quicpro_async.so' > /etc/php/8.1/cli/conf.d/99-quicpro_async.ini" > /dev/null 2>&1

clear
show_bar "php81-test: Running smoke test..." 81
output=$(docker exec -it php81-test sh -c "php /workspace/tests/utils/run-tests.php /workspace/tests/phpunit/001-smoke/test.phpt" 2>&1)
echo "$output" | grep -q "Tests passed" || log_and_fail php81-test "❌ Smoke test failed (php81-test)" "$output"

clear
show_bar "All steps completed successfully." 100
clear
message "All containers built and tested successfully!" 2 0
