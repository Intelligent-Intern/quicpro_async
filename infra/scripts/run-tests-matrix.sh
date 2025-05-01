#!/usr/bin/env bash
#───────────────────────────────────────────────────────────────────────────────
# RUN-TESTS-MATRIX.SH (php8.4 live-build edition)
# 1) Build php8.4 with live logs
# 2) Quietly ensure the rest of the images exist
# 3) Up services + PHPUnit matrix
#───────────────────────────────────────────────────────────────────────────────

set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/../../"

# load utilities
source infra/scripts/utils.sh
source infra/scripts/container-utils.sh

# 1) Live-build php8.4
message "▶ Building php84 container (live output)" 6 0
docker build \
  --progress=plain \
  -t quicpro_async.dev/php84:latest \
  ./infra/php84

# 2) Quietly ensure all the others (skip php84 since it now exists)
message "▶ Ensuring other container images are present…" 6 0
# Temporarily override the list inside ensure_containers:
ensure_containers() {
  local services=(quic-demo tshark php81 php82 php83 vue3-websocket-demo)
  local total=${#services[@]} count=0 name ctx image log

  for name in "${services[@]}"; do
    count=$((count+1))
    image="quicpro_async.dev/${name}:latest"
    log="$(mktemp /tmp/build-${name}.XXXXXX.log)"

    if ! docker image inspect "$image" >/dev/null 2>&1; then
      message "Building container: ${name}" 5 0
      case "$name" in
        quic-demo)           ctx=demo-server ;;
        tshark)              ctx=tshark ;;
        php*)                ctx="$name" ;;
        vue3-websocket-demo) ctx=vue3-websocket-demo ;;
      esac
      if docker build -t "$image" "./infra/${ctx}" >"$log" 2>&1; then
        message "Built ${name}" 10 0
      else
        message "❌ Failed building ${name}, dumping logs:" 9 15
        cat "$log"
        rm -f "$log"
        exit 1
      fi
      rm -f "$log"
    else
      message "Container exists: ${name}" 8 0
    fi

    loading_bar "$count" "$total"
  done
}

ensure_containers

# 3) Start services
message "▶ Starting core services…" 6 0
docker compose --profile test up -d quic-demo tshark vue3-websocket-demo

message "⏳ Waiting for quic-demo to become healthy…" 3 0
docker compose --profile test wait quic-demo

# 4) Run PHPUnit matrix
message "▶ Running PHPUnit (PHP 8.1 → 8.4)" 6 0
versions=(81 82 83 84)
total=${#versions[@]} count=0

for v in "${versions[@]}"; do
  count=$((count+1))
  loading_bar "$count" "$total"
  docker compose --profile test run --rm "php${v}"
done

# 5) Teardown
message "▶ Shutting down all services…" 3 0
docker compose --profile test down --remove-orphans

message "✅ All unit tests complete!" 10 0
