#!/usr/bin/env bash

pushd "${0%/*}" &>/dev/null

### check for bash debug mode
. include/debug.inc.sh

type=`basename $0`

TOOLCHAIN=""
PLATFORMNATIVE="`uname -s`"

if [ "$type" != "${type/32/}" ]; then
    [ $PLATFORMNATIVE != "FreeBSD" ] && TOOLCHAIN="i386-pc-freebsd10.0-"
    export ARCH="i386"
else
    [ $PLATFORMNATIVE != "FreeBSD" ] && TOOLCHAIN="amd64-pc-freebsd10.0-"
    export ARCH="amd64"
fi

export PLATFORM=freebsd

if [ $PLATFORMNATIVE != "FreeBSD" ]; then
    if [ -n "$USEGCC" ]; then
        export CXX="${TOOLCHAIN}g++"
        export CC="${TOOLCHAIN}gcc"
    else
        export CXX="${TOOLCHAIN}clang++"
        export CC="${TOOLCHAIN}clang"
    fi

    which $CXX &>/dev/null || {
        echo "$CXX not installed!"
        exit 1
    }
else
    # gmake uses non existing gcc
    # on FreeBSD 10 otherwise
    export CC="cc"
    export CXX="c++"
fi

export STRIP="${TOOLCHAIN}strip"

./package.sh

exit $?
