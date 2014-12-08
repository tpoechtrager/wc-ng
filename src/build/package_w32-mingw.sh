#!/usr/bin/env bash

type=`basename $0`

pushd "${0%/*}" &>/dev/null

if [[ $type == *64* ]]; then
    ARCH=x86_64
else
    ARCH=i686
fi

if [[ "`uname -s`" == *MINGW32* ]]; then
    STRIP=strip
    export JOBS=1 # https://www.google.com/search?q=msys+make+hangs
else
    MINGWPREFIX="$ARCH-w64-mingw32"
    STRIP=$MINGWPREFIX-strip

    which "${MINGWPREFIX}-g++" &>/dev/null || {
        echo "{MINGWPREFIX}-g++ not installed!"
        exit 1
    }
fi

export ARCH
export STRIP
export MINGWPREFIX

PLATFORM=mingw \
    ./package.sh $OPTS $@
