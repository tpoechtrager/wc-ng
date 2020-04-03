#!/usr/bin/env bash

pushd "${0%/*}" &>/dev/null

### variables which can be overridden
PKGDIR="${PKGDIR:-${PWD}/../..}"
PKGSUFFIX="${PKGSUFFIX:-""}"
PKGCOMPRESSOR="${PKGCOMPRESSOR:-bzip2}"

### check for bash debug mode
. include/debug.inc.sh

BUILDOPTS=""
PLATFORM=`uname -s | awk '{print tolower($0)}'`

### check if we are cross compiling

if [ "$PLATFORM" != "darwin" ]; then
    # cross compiler defaults for non darwin hosts
    eval `osxcross-conf`

    export TOOLCHAIN=x86_64-apple-$OSXCROSS_TARGET-

    export OBJCC=${TOOLCHAIN}clang
    export OBJCXX=${TOOLCHAIN}clang++

    if [ -n "$USEGCC" ]; then
        export CC=${TOOLCHAIN}gcc
        export CXX=${TOOLCHAIN}g++
    else
        if [ -n "$USELLVMGCC" ]; then
            export CC=${TOOLCHAIN}llvm-gcc
            export CXX=${TOOLCHAIN}-llvm-g++
        else
            export CC=${TOOLCHAIN}clang
            export CXX=${TOOLCHAIN}clang++
        fi
    fi

    export STRIP="${TOOLCHAIN}strip"
    export HAVECROSSCOMPILER=1

    which $CXX &>/dev/null || {
        echo "$CXX not installed!"
        exit 1
    }

    export PLATFORM="darwin"
else
    TOOLCHAIN=""
fi

### check if multiple -arch flags are supported by the compiler

if [ -z "$CXX" ]; then
    CXX=c++
fi

TESTCXXFLAGS+="$CXXFLAGS -arch i386 -arch x86_64"

# this must be in here and in build.sh
[ -z "$LTO" ] && export LTO=1

if [ -n "$LTO" -a "$LTO" != "0" ]; then
    TESTCXXFLAGS+=" `./checklto/checklto.sh $CXX`"
fi

echo "int x;" | $CXX $TESTCXXFLAGS -xc++ -c -o test - &>/dev/null

if [ $? -eq 0 -a -z "$USECCACHE" ]; then
    ARCH=`${TOOLCHAIN}lipo -info test` || exit 1
    rm test

    if [ "$ARCH" != "${ARCH/i386/}" ] && [ "$ARCH" != "${ARCH/x86_64/}" ]; then
        UNIVERSAL=1
    else
        UNIVERSAL=0
    fi
else
    UNIVERSAL=0
fi

export UNIVERSAL # export it for use in the Makefile

if [ $UNIVERSAL -eq 1 ]; then
    package_callback()
    {
        ${TOOLCHAIN}lipo wclient -extract i386 -output wclient.i386
    }
    export -f package_callback

    ./package.sh $@
else
    . include/compressor.inc.sh

    echo "need to build an extra package for each arch (will put them together at the end)"

    set -e

    PTMP=`mktemp -d /tmp/XXXXXXXXX`

    test -n "$USELLVMGCC" && export CXXFLAGS=" -Wno-unused-variable"

    SUFFIX=$PKGSUFFIX

    export PKGSUFFIX="_i386${SUFFIX}"
    TARGETS="-m32" PKGDIR=$PTMP ./package.sh $BUILDOPTS $@

    export PKGSUFFIX="_x86_64${SUFFIX}"
    TARGETS="-m64" PKGDIR=$PTMP ./package.sh $BUILDOPTS $@

    version=`./get_version.sh`
    sauer_svn_rev=`./get_sauer_svn_rev.sh`

    X32PKG="${PTMP}/wc-ng-v${version}-sr${sauer_svn_rev}_darwin_i386${SUFFIX}.tar.bz2"
    X64PKG="${PTMP}/wc-ng-v${version}-sr${sauer_svn_rev}_darwin_x86_64${SUFFIX}.tar.bz2"

    TMP=`mktemp -d /tmp/XXXXXXXXX`

    pushd $TMP &>/dev/null

    echo "extracting 32 bit binaries"
    tar -xjf $X32PKG -C $TMP plugins wclient

    mv wclient wclient.i386

    pushd plugins &>/dev/null
    for f in *.dylib; do
        mv $f $f.i386
    done
    popd &>/dev/null

    ### TODO: support other compressors
    echo "extracting 64 bit binaries"
    tar -xjf $X64PKG -C $TMP plugins wclient

    mv wclient wclient.x86_64

    pushd plugins &>/dev/null
    for f in *.dylib; do
        mv $f $f.x86_64
    done
    popd &>/dev/null

    echo "creating fat mach-o binaries"

    ${TOOLCHAIN}lipo -create wclient.i386 wclient.x86_64 -output wclient

    pushd plugins &>/dev/null
    for f in *.x86_64; do
        X32="${f/x86_64/i386}"
        ${TOOLCHAIN}lipo -create $X32 $f -output "${f/\.x86_64/}"
    done
    popd &>/dev/null

    rm wclient.x86_64
    rm plugins/*.dylib.*

    popd &>/dev/null

    CTMP=`mktemp -d /tmp/XXXXXXXXX`

    tar xf $X64PKG -C $CTMP

    rm $CTMP/wclient
    rm -r $CTMP/plugins

    mv $TMP/* $CTMP

    echo "creating final package"

    ### TODO: get rid of this duplicate code

    test -d "${PKGDIR}" || mkdir -p "${PKGDIR}"
    PKGNAME="wc-ng-v${version}-sr${sauer_svn_rev}_${PLATFORM}"
    PKGNAME="${PKGNAME}${SUFFIX}${PKGEXT}"

    pushd $CTMP &>/dev/null
    compress "*" "${PKGDIR}/$PKGNAME"
    popd &>/dev/null

    if [ -n "$SIGNPACKAGE" ]; then
        pushd "$PKGDIR" &>/dev/null
        gpg --sign --detach-sign --armor "$PKGNAME"
        popd &>/dev/null
    fi

    rm -rf $TMP
    rm -rf $CTMP
    rm -rf $PTMP
fi
