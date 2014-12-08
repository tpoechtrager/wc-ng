#!/usr/bin/env bash

export LC_ALL="C"
export LANG=""

pushd ${0%/*} &>/dev/null

### check for bash debug mode
. include/debug.inc.sh

src="../mod/wcversion.h.in"
dst="../mod/wcversion.h"

test "$1" = "clean" && { rm -f "$dst"; exit 0; }

which git &>/dev/null
test $? -ne 0 && echo "git not installed?" 2>&1 && exit 1

REVISION=$(git log --pretty=format:'%h' -n 1)

test "$1" = "stdout" && echo $REVISION && exit 0

test -f "$dst" && grep "#define WCREVISION \"${REVISION}\"" "$dst" &>/dev/null && exit 0
cp "$src" "$dst"
sed -i'' -e 's/"$WCREVISION$"/"'$REVISION'"/g' "$dst"
