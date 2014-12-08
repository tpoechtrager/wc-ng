#!/usr/bin/env bash

type=`basename $0`

pushd "${0%/*}" &>/dev/null

which wclang >/dev/null || exit 1

export PLATFORM=windows
export MINGWPREFIX=`w32-clang -wc-target`
export MINGWCLANGPREFIX=w32-clang
export MINGWARCH=i686
export CFLAGS="-fms-extensions -D_MSC_VER=1800"
export CXXFLAGS="-fms-extensions -D_MSC_VER=1800"

./checksyntax.sh $@

