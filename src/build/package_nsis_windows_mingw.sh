#!/usr/bin/env bash

# creates a windows installer on unix/linux using nsis
# requires nsis, mingw-w64 or clang (depending on call name)

pushd "${0%/*}" &>/dev/null

### check for bash debug mode
. include/debug.inc.sh

### variables which can be overridden
PKGDIR="${PKGDIR:-${PWD}/../..}"

type="$(basename $0)"

which makensis &>/dev/null || exit 1

cleanup()
{
    test -n "$tmpdir" -a -d "$tmpdir" && rm -rf "$tmpdir"
}

trap "cleanup" EXIT
set -e

export PKGCOMPRESSION="zip"

version=`./get_version.sh`

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
unzip -o "${PKGDIR}/wc-ng-v${version}_windows_i*${PKGSUFFIX}.zip"
unzip -o "${PKGDIR}/wc-ng-v${version}_windows_x*${PKGSUFFIX}.zip"


# remove msvc method of getting the git revision
awk '!((NR==3)||(NR==4)||(NR==5)||(NR==6)||(NR==7))' < wc.nsi > tmp.nsi
mv tmp.nsi wc.nsi

if [[ "`uname -s`" != *MINGW32* ]]; then
    sed -i "s/\${VERSION}/$version/g" wc.nsi
    sed -i 's/\.\.\\\.\.\\\.\.\\//g' wc.nsi
    sed -i 's/\.\.\\\.\.\\//g' wc.nsi
    sed -i "s/_windows_setup.exe/_windows${PKGSUFFIX}_setup.exe/g" wc.nsi
fi

makensis wc.nsi
mv *.exe "${PKGDIR}/"

if [ -n "$SIGNPACKAGE" ]; then
    pushd "$PKGDIR" &>/dev/null
    gpg --sign --detach-sign --armor wc-ng*v${version}*${PKGSUFFIX}*.exe
    popd &>/dev/null
fi
