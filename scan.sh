#!/bin/bash

BASE_URL="https://intelligentinternpecl.z1.web.core.windows.net"
PKG_NAME="quicpro_async"
VERSION="0.1.0"

FILES=(
  "/channel.xml"
  "/rest/p/packages.xml"
  "/rest/p/${PKG_NAME}/info.xml"
  "/rest/r/${PKG_NAME}/allreleases.xml"
  "/rest/r/${PKG_NAME}/latest.txt"
  "/rest/r/${PKG_NAME}/stable.txt"
  "/rest/r/${PKG_NAME}/${VERSION}.xml"
  "/rest/r/${PKG_NAME}/${VERSION}.deps.xml"
  "/get/${PKG_NAME}-${VERSION}.tgz"
)

echo "== Checking PECL REST structure for package: ${PKG_NAME} (${VERSION}) =="
echo

for file in "${FILES[@]}"; do
  url="${BASE_URL}${file}"
  status=$(curl -s -o /tmp/pecltest.tmp -w "%{http_code}" "$url")
  head=$(head -n 1 /tmp/pecltest.tmp)
  printf "%-60s [HTTP %s]   %s\n" "$file" "$status" "$head"
done

rm -f /tmp/pecltest.tmp
