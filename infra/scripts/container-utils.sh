#!/usr/bin/env bash
#───────────────────────────────────────────────────────────────────────────────
# CONTAINER-UTILS.SH
# ensure_containers(): build any missing images, quietly on success,
# but dump logs on failure for easy debugging.
#───────────────────────────────────────────────────────────────────────────────

ensure_containers() {
    local services=(quic-demo tshark php81 php82 php83 php84 vue3-websocket-demo)
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

            # build quietly, capture logs
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
