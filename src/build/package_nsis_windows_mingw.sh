#!/usr/bin/env bash

# creates a windows installer on unix/linux using nsis
# requires nsis, mingw-w64 or clang (depending on call name)

pushd "${0%/*}" &>/dev/null

### check for bash debug mode
. include/debug.inc.sh

### variables which can be overridden
PKGDIR="${PKGDIR:-${PWD}/../..}"

type="$(basename $0)"

if [[ $(uname -s) == CYGWIN* ]]; then
    if [ -d "/cygdrive/c/Program Files (x86)/NSIS" ]; then
        PATH="$PATH:/cygdrive/c/Program Files (x86)/NSIS:"
    elif [ -d "/cygdrive/c/Program Files/NSIS" ]; then
        PATH="$PATH:/cygdrive/c/Program Files/NSIS:"
    fi
fi

which makensis &>/dev/null || { echo "nsis is required" 1>&2; exit 1; }

cleanup()
{
    test -n "$tmpdir" -a -d "$tmpdir" && rm -rf "$tmpdir"
}

trap "cleanup" EXIT
set -e

export PKGCOMPRESSION="zip"

version=`./get_version.sh`
sauer_svn_rev=`./get_sauer_svn_rev.sh`

if [ "$type" != "${type/clang/}" ]; then
    ./package_w32-clang.sh $@
    ./package_w64-clang.sh $@
else
    ./package_w32-mingw.sh $@
    ./package_w64-mingw.sh $@
fi

tmpdir=`mktemp -d`

cp -p wc.nsi $tmpdir/

pushd $tmpdir &>/dev/null
unzip -o "${PKGDIR}/wc-ng-v${version}-sr${sauer_svn_rev}_windows_i*${PKGSUFFIX}.zip"
unzip -o "${PKGDIR}/wc-ng-v${version}-sr${sauer_svn_rev}_windows_x*${PKGSUFFIX}.zip"


# remove msvc method of getting the git revision
awk '!((NR==3)||(NR==4)||(NR==5)||(NR==6)||(NR==7))' < wc.nsi > tmp.nsi
mv tmp.nsi wc.nsi

if [[ "`uname -s`" != *MINGW32* ]]; then
    sed -i "s/\${VERSION}/${version}-sr${sauer_svn_rev}/g" wc.nsi
    sed -i 's/\.\.\\\.\.\\\.\.\\//g' wc.nsi
    sed -i 's/\.\.\\\.\.\\//g' wc.nsi
    sed -i "s/_windows_setup.exe/_windows${PKGSUFFIX}_setup.exe/g" wc.nsi
fi

sh -c "makensis wc.nsi" # cygwin's bash shell is doing weird things without "sh -c"
mv *.exe "${PKGDIR}/"

if [ -n "$SIGNPACKAGE" ]; then
    pushd "$PKGDIR" &>/dev/null
    gpg --sign --detach-sign --armor wc-ng*v${version}_sr${sauer_svn_rev}*${PKGSUFFIX}*.exe
    popd &>/dev/null
fi
