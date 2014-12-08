#!/usr/bin/env bash

pushd "${0%/*}" &>/dev/null

if [ -z "$JOBS" ]; then
    export JOBS=$(./get_cpu_count.sh)
    test $? -ne 0 && exit 1
fi

pushd .. &>/dev/null

[ -z "$MAKEFILE" ] && MAKEFILE=Makefile

# use gnu make
if [ -z "$MAKE" ]; then
    MAKE=make
    [[ `uname -s` == *BSD ]] && MAKE=gmake
fi

[ -z "$LTO" ] && export LTO=1

$MAKE -f $MAKEFILE $* -j$JOBS $@

exit $?
