#!/usr/bin/env bash

type=`basename $0`

pushd "${0%/*}" &>/dev/null

if [[ $type == *w32* ]]; then
    type=i686
else
    type=x86_64
fi

which wclang &>/dev/null || { echo "wclang not installed"; exit 1; }

MINGWPREFIX=`$type-w64-mingw32-clang++ -wc-target` \
MINGWCLANGPREFIX=$MINGWPREFIX-clang \
ARCH=$type \
PLATFORM=mingw \
STRIP=`$MINGWCLANGPREFIX -wc-env-strip` \
    ./package.sh $OPTS $@
