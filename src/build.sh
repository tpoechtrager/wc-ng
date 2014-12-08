#!/usr/bin/env bash

set -e

pushd "${0%/*}" &>/dev/null

# force english warning/error messages
export LC_ALL="C"

# check if we are on a supported host
PLATFORM=$(uname -s)
case $PLATFORM in
     Linux|Darwin|*BSD)
         ;;
     *)
        echo "this script is unix only for now" >&2
        exit 1
        ;;
esac

# prefer clang when CXX/CC are not set
test -z "$CXX" && which clang++ &>/dev/null && export CXX="clang++"
test -z "$CC" && which clang &>/dev/null && export CC="clang"

# optimize for host architecture
export CFLAGS+=" -march=native"
export CXXFLAGS+=" -march=native"

# sanitizers
if [ -n "$ASAN" ]; then
  export CFLAGS+=" -fsanitize=address"
  export CXXFLAGS+=" -fsanitize=address -DUSE_STD_NEW"
fi

if [ -n "$UBSAN" ]; then
  export CFLAGS+=" -fsanitize=undefined"
  export CXXFLAGS+=" -fsanitize=undefined"
fi

# append WCCXXFLAGS to CXXFLAGS
test -n "$WCCXXFLAGS" && CXXFLAGS+=" $WCCXXFLAGS"

# exit on error
set -e

# export library paths
if [ -n "$WCLIBDIR" ]; then
    export CPLUS_INCLUDE_PATH="$WCLIBDIR/include:$CPLUS_INCLUDE_PATH"
    export LIBRARY_PATH="$WCLIBDIR/lib:$LIBRARY_PATH"
fi

# use lto?
if [ -z "$LTO" ]; then
    if [[ $0 == *lto.sh ]]; then
        LTO=1
    else
        LTO=0
    fi
    export LTO
fi

# build
./build/build.sh $@

# install
if [ -n "$WCINSTALLDIR" ]; then
    if [ -e "$WCINSTALLDIR" -a ! -d "$WCINSTALLDIR" ]; then
        echo "install dir: $WCINSTALLDIR exists but is not a directory" >&2
        exit 1
    fi

    if [ $PLATFORM == "Darwin" ]; then
        # Assume Sauerbraten.app
        mkdir -p $WCINSTALLDIR/Contents/gamedata/{packages,data,doc,plugins}
        cp -p wclient "$WCINSTALLDIR/Contents/MacOS/sauerbraten_u"
        cp -p plugins/*.dylib "$WCINSTALLDIR/Contents/gamedata/plugins/"
        cp -pr ../data/* "$WCINSTALLDIR/Contents/gamedata/data"
        cp -pr ../packages/* "$WCINSTALLDIR/Contents/gamedata/packages"
        cp -pr ../doc/* "$WCINSTALLDIR/Contents/gamedata/doc"
    else
        mkdir -p $WCINSTALLDIR/{bin_unix,packages,data,doc,plugins}
        cp -p sauer_client "$WCINSTALLDIR/bin_unix/"
        cp -p plugins/*.so "$WCINSTALLDIR/plugins/"
        cp -pr ../data/* "$WCINSTALLDIR/data/"
        cp -pr ../packages/* "$WCINSTALLDIR/packages/"
        cp -pr ../doc/* "$WCINSTALLDIR/doc/"
    fi
fi
