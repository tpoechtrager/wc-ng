#!/usr/bin/env bash

pushd "${0%/*}" &>/dev/null

set -e

CACHEDIR=.cache
mkdir -p $CACHEDIR

X=`echo "$*" | sed "s/\//_/g"`
CACHEFILE="$CACHEDIR/${X}_enet_cflags_cache"

if [ ! -f "$CACHEFILE" ]; then
    ../enet/check_cflags.sh $* > "$CACHEFILE"
fi

cat "$CACHEFILE"
