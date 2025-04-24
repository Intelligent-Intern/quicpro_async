#!/bin/bash
set -euo pipefail

update_release_date() {
  local today
  today=$(date -u '+%Y-%m-%d')
  sed -i "s|<date>.*</date>|<date>${today}</date>|" package.xml
  echo "✔ Release date updated to ${today}"
}

run_act_publish() {
  bin/act -j publish -P ubuntu-latest=catthehacker/ubuntu:act-latest
}

main() {
  update_release_date
  run_act_publish
}

main "$@"
