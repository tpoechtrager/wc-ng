#!/usr/bin/env bash

pushd "${0%/*}" &>/dev/null

### check for bash debug mode
. include/debug.inc.sh

if [ $(uname -s) != "Linux" ]; then
    echo "This script must be run on a linux host!"
    exit 1
fi

if [[ $(basename $0) == *32* ]]; then
    ARCH="i686"
else
    ARCH="x86_64"
fi

if [[ "$CC" == *icc* ]]; then
    export COMPATCC=gcc
fi

GLIBCCOMPAT=1 \
ARCH=$ARCH \
  ./package.sh

exit $?
