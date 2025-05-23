#!/usr/bin/env bash
#───────────────────────────────────────────────────────────────────────────────
# CONTAINER-UTILS.SH
# Dedicated build functions for each container; using general-purpose messaging and loading helpers.
#───────────────────────────────────────────────────────────────────────────────

# Build QUIC demo server container
build_quic_demo() {
    build_container "quic-demo" "infra/demo-server" "quicpro_async.dev/quic-demo:latest"
}

# Build Tshark container
build_tshark() {
    build_container "tshark" "infra/tshark" "quicpro_async.dev/tshark:latest"
}

# Build PHP 8.1 container
build_php81() {
    build_container "php81" "infra/php81" "quicpro_async.dev/php81:latest"
}

# Build PHP 8.2 container
build_php82() {
    build_container "php82" "infra/php82" "quicpro_async.dev/php82:latest"
}

# Build PHP 8.3 container
build_php83() {
    build_container "php83" "infra/php83" "quicpro_async.dev/php83:latest"
}

# Build PHP 8.4 container
build_php84() {
    build_container "php84" "infra/php84" "quicpro_async.dev/php84:latest"
}

# Build Vue3 WebSocket demo container
build_vue3_websocket_demo() {
    build_container "vue3-websocket-demo" "infra/vue3-websocket-demo" "quicpro_async.dev/vue3-websocket-demo:latest"
}

# Core build logic with messaging and loading bar
build_container() {
    local name="$1" ctx="$2" image="$3"
    local logfile
    logfile=$(mktemp /tmp/build-"${name}".XXXXXX.log)
    if docker build -t "${image}" "${ctx}" >"${logfile}" 2>&1; then
        message "Built ${name}" 10 0
    else
        message "❌ Failed building ${name}" 9 0
        message "Dumping logs for ${name}:" 9 15
        cat "${logfile}"
        rm -f "${logfile}"
        exit 1
    fi
    rm -f "$logfile"
}

# Public function: call individual builders as needed
ensure_containers() {
    build_quic_demo
    build_php81
    build_php82
    build_php83
    build_php84
#    build_tshark
#    build_vue3_websocket_demo
}
