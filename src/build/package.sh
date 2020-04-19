#!/usr/bin/env bash

pushd "${0%/*}" &>/dev/null

### common tools
. include/common.inc.sh

### check for bash debug mode
. include/debug.inc.sh

### variables which can be overridden
PKGDIR="${PKGDIR:-${PWD}/../..}"
PKGSUFFIX="${PKGSUFFIX:-""}"
PKGCOMPRESSOR="${PKGCOMPRESSOR:-""}"


### clean up on exit
cleanup()
{
    test -n "$doctmp" -a -d "$doctmp" && rm -rf "$doctmp"
    test -n "$packagetmp" -a -d "$packagetmp" && rm -rf "$packagetmp"
    pushd $SRCDIR &>/dev/null && test -n "$MAKEFILE" -a -f "$MAKEFILE" && \
      $MAKE -f "$MAKEFILE" clean
}

trap "cleanup" EXIT

### exit in case any subcommand failed
set -e

SRCDIR="${PWD}/.."

version=`./get_version.sh`
sauer_svn_rev=`./get_sauer_svn_rev.sh`

export MAKEFILE=Makefile

STRIPARGS=""

[ -z "$PLATFORM" ] && PLATFORM=`uname -s | awk '{print tolower($0)}'`
[ -z "$NATIVEPLATFORM" ] && NATIVEPLATFORM=`uname -s | awk '{print tolower($0)}'`
[ -z "$ARCH" ] && ARCH=`uname -m`
[ -n "$DEBUG" ] && PKGSUFFIX+="_dbg"
[ -z "$STRIP" ] && STRIP=strip

if [ "$PLATFORM" == "mingw" ]; then
    MINGW=1
    which unix2dos &>/dev/null || { echo "unix2dos is not installed" >&2; }
else
    MINGW=0
    which dos2unix &>/dev/null || { echo "dos2unix is not installed" >&2; }
fi

if [ -z "$MAKE" ]; then
    if [[ $NATIVEPLATFORM == *bsd ]]; then
        MAKE=gmake
    else
        MAKE=make
    fi
    export MAKE
fi

if [ $MINGW -ne 0 ]; then
    SAUERBIN="sauerbraten_wc.exe"
    CHATSERVERBIN="wc_chat_server.exe"
    PLUGINS="plugins/*.dll"
    PKGCOMPRESSOR="${PKGCOMPRESSOR:-zip}"

    if [ $ARCH == "x86_64" ]; then
        BINDIR="bin64"
    else
        BINDIR="bin"
    fi
else
    PKGCOMPRESSOR="${PKGCOMPRESSOR:-bzip2}"
    CHATSERVERBIN="wc_chat_server"
    if [ "$PLATFORM" == "darwin" ]; then
        SAUERBIN="wclient*"
        PLUGINS="plugins/*.dylib"
        BINDIR=""
        STRIPARGS="-x"
    else
        SAUERBIN="sauer_client"
        PLUGINS="plugins/*.so"
        BINDIR="bin_unix"
    fi
fi

. include/compressor.inc.sh

pushd "$SRCDIR" &>/dev/null
$MAKE -f $MAKEFILE clean 2>/dev/null
popd &>/dev/null

./build.sh $@ USESTATICLIBS=1

if [ -z "$DEBUG" ]; then
    $STRIP $STRIPARGS ${SRCDIR}/$SAUERBIN
    $STRIP $STRIPARGS ${SRCDIR}/$CHATSERVERBIN
fi

set +e
if [ "`ls ${SRCDIR}/$PLUGINS 2>/dev/null | wc -l | tr -d ' '`" -ge 1 ]; then
    [ -z "$DEBUG" ] && $STRIP $STRIPARGS ${SRCDIR}/$PLUGINS
    HAVE_PLUGINS=1
else
    HAVE_PLUGINS=0
fi
set -e

packagetmp=`mktemp -d /tmp/XXXXXXXXX`
mkdir -p $packagetmp/$BINDIR
mkdir $packagetmp/plugins
mkdir $packagetmp/doc

cp -p ${SRCDIR}/$SAUERBIN $packagetmp/$BINDIR
cp -p ${SRCDIR}/$CHATSERVERBIN $packagetmp/$BINDIR

[ $HAVE_PLUGINS -eq 1 ] && cp -p \
  ${SRCDIR}/$PLUGINS $packagetmp/plugins

cp -pr ${SRCDIR}/../packages $packagetmp
cp -pr ${SRCDIR}/../data $packagetmp

doctmp=`mktemp -d /tmp/XXXXXXXXX`
cp -pr ${SRCDIR}/../doc/* ${doctmp}/

pushd $doctmp &>/dev/null
if [ $MINGW -ne 0 ]; then
    find . -type f -exec unix2dos {} 2>/dev/null \;
else
    find . -type f -exec dos2unix {} 2>/dev/null \;
fi
popd &>/dev/null

cp -r $doctmp/licenses $packagetmp/doc
cp -p $doctmp/style.css $packagetmp/doc
cp -pr $doctmp/examples $packagetmp/doc
cp -p $doctmp/WC_README.html $packagetmp/doc/WC_README.html
cp -p $doctmp/*.cfg $packagetmp/doc

if [ $MINGW -ne 0 ]; then
    if [ $ARCH == "x86_64" ]; then
        find $SRCDIR ! -regex ".*[/]\x86_64\/[/]?.*" -type f -o -name '*.dll' \
          -exec cp {} $packagetmp/$BINDIR \;
    else
        find $SRCDIR ! -regex ".*[/]\i686\/[/]?.*" -type f -o -name '*.dll' \
          -exec cp {} $packagetmp/$BINDIR \;
    fi
    rm -f $packagetmp/$BINDIR/mingw-crt-x*.dll
fi

test -d "${PKGDIR}" || mkdir -p "${PKGDIR}"
PLATFORM_SUFFIX=$PLATFORM
[ $PLATFORM == "mingw" ] && PLATFORM_SUFFIX="windows"
[ $PLATFORM == "darwin" ] && PLATFORM_SUFFIX="macos"

PKGNAME="wc-ng-v${version}-sr${sauer_svn_rev}_${PLATFORM_SUFFIX}"

if [ "$PLATFORM" != "darwin" ]; then
    # assume it's not a fat binary
    PKGNAME="${PKGNAME}_${ARCH}"
fi

PKGNAME="${PKGNAME}${PKGSUFFIX}${PKGEXT}"

pushd "$packagetmp" &>/dev/null
set +e
find . -name ".DS_Store" -exec rm -rf {} \; &>/dev/null
find . -type d -name ".svn" -exec rm -rf {} \; &>/dev/null
find . -type d -name ".git" -exec rm -rf {} \; &>/dev/null
set -e

declare -f -F package_callback &>/dev/null && package_callback

compress "*" "${PKGDIR}/$PKGNAME"
