#!/usr/bin/env bash
set -e
pushd "${0%/*}/.." &>/dev/null
make clean
make server -j$(./build/get_cpu_count.sh) USESTATICLIBS=1 LTO=1 DEBUG=3
SUFFIX=""
if [[ $(ls -t | grep sauer_server | head -n1) == *.exe ]]; then
  SUFFIX=".exe"
fi
BIN="sauer_server.r$(grep SAUERSVNREV last_sauer_svn_rev.h | awk '{print $3}')"
mv "sauer_server$SUFFIX" "$BIN$SUFFIX"
echo "done: $BIN$SUFFIX"
