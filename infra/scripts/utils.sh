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
    local current=$1 total=$2 width=${3:-40} filled empty
    filled=$(( (current * width) / total ))
    empty=$(( width - filled ))
    printf "\r["; printf "%0.s#" $(seq 1 $filled)
    printf "%0.s " $(seq 1 $empty); printf "] %d/%d" "$current" "$total"
    if [ "$current" -eq "$total" ]; then echo; fi
}

follow_logs_until() {
    local ctr="$1" term="$2"
    message "Following logs for ${ctr} until ‘${term}’ appears…" 12 0
    ( docker logs -f "$ctr" 2>&1 | grep --line-buffered -q "$term" ) &
    wait $!
    message "Term ‘${term}’ found in ${ctr}" 10 0
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
