#!/usr/bin/env bash

pushd "${0%/*}" &>/dev/null

function check()
{
    $COMPILER $@ -xc 1.c 2.c -o test &>/dev/null
    if [ $? -eq 0 ]; then
        echo "$@ -D__lto__"
        exit
    fi
}

function checklto()
{
    local str=`$COMPILER --version`

    if [[ $str == *Free\ Software\ Foundation* ]]; then
        check -flto=$JOBS -fno-fat-lto-objects
        check -flto -fno-fat-lto-objects
        check -flto=$JOBS
        check -flto
    elif [[ $str == *clang* ]] || [[ $str == *LLVM* ]]; then
        check -flto
    elif [[ $str == *ICC* ]]; then
        check -ipo -wd11021
    fi

    echo "link time optimization not supported" 1>&2
}

[ -z "$JOBS" ] && JOBS=1

COMPILER="$@"

checklto
