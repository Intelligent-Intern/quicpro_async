#!/usr/bin/env bash
#───────────────────────────────────────────────────────────────────────────────
# General-purpose messaging, loading bar, and log-follow helpers for CI scripts
#
# COLORS (tput setab/setaf indices):
#  1 = orange        2 = grey green    3 = yellow
#  4 = grey blue     5 = light purple  6 = mint
#  7 = snow          8 = grey          9 = red (errors)
# 10 = green (ok)   11 = orange (warn) 12 = light blue
# 13 = pink         14 = light green   15 = white
# 16 = black        17 = dark blue     18 = med dark blue
# 19 = blue
#
# USAGE:
#   source utils.sh
#   message "Hello world" [bgcolor] [fgcolor]
#   loading_bar current total [width]
#   follow_logs_until container "search string"
#
#───────────────────────────────────────────────────────────────────────────────

message() {
    local str="$1" bgcolor="${2:-0}" color="${3:-7}" boxwidth term_width \
          bgcmd colorcmd normal
    term_width=$(tput cols 2>/dev/null || echo 80)
    boxwidth=${4:-$term_width}
    bgcmd=$(tput setab "$bgcolor") colorcmd=$(tput setaf "$color")
    normal=$(tput sgr0)

    echo
    # top border
    echo -ne "${bgcmd}"; printf "%*s" "$boxwidth" "" | tr ' ' ' '
    # centered message
    echo -ne "${bgcmd}${colorcmd}"
    printf "%*s%s%*s" \
      $(( (boxwidth - ${#str}) / 2 )) "" "$str" $(( (boxwidth - ${#str} + 1) / 2 )) ""
    # bottom border
    echo -ne "${bgcmd}"; printf "%*s" "$boxwidth" "" | tr ' ' ' '
    echo -e "${normal}"
    sleep 0.2
}

loading_bar() {
    local progress=$1 total=$2 width=${3:-40} done=$(( progress * width / total )) \
          left=$(( width - done )) percent=$(( 100 * progress / total ))
    printf "\r["; printf "%0.s#" $(seq 1 $done); printf "%0.s-" $(seq 1 $left)
    printf "] %s%%" "$percent"
    if (( progress == total )); then echo; fi
}

follow_logs_until() {
    local container=$1 pattern=$2
    docker logs -f "$container" 2>&1 | grep --line-buffered -m1 "$pattern"
}

ask_user() {
    local question="$1" response
    local green="\033[1;32m" red="\033[1;31m" yellow="\033[1;33m" reset="\033[0m"
    while true; do
        echo -e "${yellow}$question${reset} ${green}[y]${reset}/${red}[n]${reset}: \c"
        read -r response
        case "$response" in
            [Yy]* ) return 0 ;;
            [Nn]* ) return 1 ;;
            * ) echo -e "${red}Invalid input. Please enter 'y' or 'n'.${reset}" ;;
        esac
    done
}

check_and_install_mkcert() {
    sudo apt install -y libnss3-tools mkcert
    mkcert -install
}

update_certs() {
    message "Creating local TLS certificates for .dev and .local domains for testing"
    local compose_file="./docker-compose.yml"
    local certs_dir="./var/traefik/certificates"
    mkdir -p "$certs_dir"
    local ca_pem
    ca_pem="$(mkcert -CAROOT)/rootCA.pem"
    mapfile -t domains < <(grep -oP 'Host\(`\K[^`]*' "$compose_file")
    mkcert -cert-file "$certs_dir/local-cert.pem" -key-file "$certs_dir/local-key.pem" "${domains[@]}" > /dev/null 2>&1
    sudo cp "$ca_pem" /usr/local/share/ca-certificates/mkcert-rootCA.crt > /dev/null 2>&1
    sudo update-ca-certificates > /dev/null 2>&1
    chmod 644 "$certs_dir/local-cert.pem" > /dev/null 2>&1
    chmod 600 "$certs_dir/local-key.pem" > /dev/null 2>&1
}

update_hosts() {
  mapfile -t domains < <(grep -oP 'Host\(`\K[^`]+' "$DOCKER_COMPOSE_FILE")
  for d in "${domains[@]}"; do
      grep -q "127.0.0.1 $d" /etc/hosts || echo "127.0.0.1 $d" | sudo tee -a /etc/hosts >/dev/null
  done
  message "Updated /etc/hosts" 10 0

}

# Style 1: Minimal single-line underline
help_box() {
  local title="$1"; shift; local items=("$@")
  echo
  echo "$title"
  printf '%*s\n' "${#title}" '' | tr ' ' '-'
  for it in "${items[@]}"; do
    printf "  • %s\n" "$it"
  done
  echo
}

