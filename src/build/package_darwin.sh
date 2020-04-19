#!/usr/bin/env bash

pushd "${0%/*}" &>/dev/null

### variables which can be overridden
PKGDIR="${PKGDIR:-${PWD}/../..}"
PKGSUFFIX="${PKGSUFFIX:-""}"
PKGCOMPRESSOR="${PKGCOMPRESSOR:-bzip2}"

### check for bash debug mode
. include/debug.inc.sh

BUILDOPTS=""
export PLATFORM="darwin"

which xcrun &>/dev/null || {
    echo "xcrun not installed!"
    exit 1
}

export OBJCC="xcrun clang"
export OBJCXX="xcrun clang++"

if [ -n "$USEGCC" ]; then
    export CC="xcrun gcc"
    export CXX="xcrun g++"
else
    export CC="xcrun clang"
    export CXX="xcrun clang++"
fi

export STRIP="xcrun strip"

# this must be in here and in build.sh
[ -z "$LTO" ] && export LTO=1

. include/compressor.inc.sh

I386_SUPPORTED=0
X64_SUPPORTED=0
X64H_SUPPORTED=0

echo "int main(){return 0;}" | xcrun $CC -m32 -xc - &>/dev/null && I386_SUPPORTED=1
echo "int main(){return 0;}" | xcrun $CC -m64 -xc - &>/dev/null && X64_SUPPORTED=1
echo "int main(){return 0;}" | xcrun $CC -arch x86_64h -xc - &>/dev/null && X64H_SUPPORTED=1

echo -n "Archs supported: "
echo "i386: $I386_SUPPORTED, x86_64: $X64_SUPPORTED, x86_64h: $X64H_SUPPORTED"
echo ""

NUM_SUPPORTED_ARCHS=$(($I386_SUPPORTED + $X64_SUPPORTED + $X64H_SUPPORTED))

set -e

if [ $I386_SUPPORTED -eq 1 ]; then
    TARGETS="-m32" PKGSUFFIX="_i386${PKGSUFFIX}" ./package.sh $BUILDOPTS $@
fi

if [ $X64_SUPPORTED -eq 1 ]; then
    TARGETS="-m64" PKGSUFFIX="_x86_64${PKGSUFFIX}" ./package.sh $BUILDOPTS $@
fi

if [ $X64H_SUPPORTED -eq 1 ]; then
    TARGETS="-arch x86_64h" PKGSUFFIX="_x86_64h${PKGSUFFIX}" ./package.sh $BUILDOPTS $@
fi

if [ $NUM_SUPPORTED_ARCHS -eq 1 ]; then
    exit 0
fi

version=$(./get_version.sh)
sauer_svn_rev=$(./get_sauer_svn_rev.sh)

X32PKG="${PKGDIR}/wc-ng-v${version}-sr${sauer_svn_rev}_macos_i386${PKGSUFFIX}.tar.bz2"
X64PKG="${PKGDIR}/wc-ng-v${version}-sr${sauer_svn_rev}_macos_x86_64${PKGSUFFIX}.tar.bz2"
X64HPKG="${PKGDIR}/wc-ng-v${version}-sr${sauer_svn_rev}_macos_x86_64h${PKGSUFFIX}.tar.bz2"

TMP=$(mktemp -d /tmp/XXXXXXXXX)

pushd $TMP &>/dev/null

extract_package()
{
    echo "extracting $2 binaries"
    tar -xjf $1 -C $TMP plugins wclient wc_chat_server

    mv wclient wclient.$2
    mv wc_chat_server wc_chat_server.$2

    pushd plugins &>/dev/null
    for f in *.dylib; do
        mv $f $f.$2
    done
    popd &>/dev/null
}

if [ $I386_SUPPORTED -eq 1 ]; then
    extract_package $X32PKG i386
fi

if [ $X64_SUPPORTED -eq 1 ]; then
    extract_package $X64PKG x86_64
fi

if [ $X64H_SUPPORTED -eq 1 ]; then
    extract_package $X64HPKG x86_64h
fi

echo "creating fat mach-o binaries"

supported_arch=""

create_fat_binary()
{
    local files=""

    if [ $I386_SUPPORTED -eq 1 ]; then
        files+="$1.i386 "
        supported_arch="i386"
    fi

    if [ $X64_SUPPORTED -eq 1 ]; then
        files+="$1.x86_64 "
        supported_arch="x86_64"
    fi

    if [ $X64H_SUPPORTED -eq 1 ]; then
        files+="$1.x86_64h "
        supported_arch="x86_64h"
    fi

    xcrun lipo -create $files -output $1
    rm -f $1.i386 $1.x86_64 $1.x86_64h
}

create_fat_binary wclient
create_fat_binary wc_chat_server

pushd plugins &>/dev/null
for file in *.$supported_arch; do
    name=$(echo -n $f|tr '.' '\n'|head -n2|tr '\n' '.')
    files=""
    [ $I386_SUPPORTED -eq 1 ] && files+="$name.i386 "
    [ $X64_SUPPORTED -eq 1 ] && files+="$name.x86_64 "
    [ $X64H_SUPPORTED -eq 1 ] && files+="$name.x86_64h "
    xcrun lipo -create $files -output $name
    rm -f $files
done
popd &>/dev/null # plugins

popd &>/dev/null

CTMP=$(mktemp -d /tmp/XXXXXXXXX)

tar xf $X64PKG -C $CTMP

rm $CTMP/wclient
rm -r $CTMP/plugins

mv $TMP/* $CTMP

echo "creating final package"

test -d "${PKGDIR}" || mkdir -p "${PKGDIR}"
PKGNAME="wc-ng-v${version}-sr${sauer_svn_rev}_macos_universal"
PKGNAME="${PKGNAME}${PKGSUFFIX}${PKGEXT}"

pushd $CTMP &>/dev/null
compress "*" "${PKGDIR}/$PKGNAME"
popd &>/dev/null

rm -rf $TMP
rm -rf $CTMP
rm -rf $PTMP

