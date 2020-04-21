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
COMMIT_COUNT=$(git rev-list HEAD --count)

cp "$src" "$dst.new"
sed -i'' -e "s/__GIT_REVISION__/$REVISION/g" "$dst.new"
sed -i'' -e "s/__GIT_COMMIT_COUNT__/${COMMIT_COUNT}/g" "$dst.new"

if [ ! -f "$dst" ]; then
    mv "$dst.new" "$dst"
elif cmp "$dest.new" "$dst" 2>/dev/null; then
    mv "$dst.new" "$dst"
fi

rm -f "$dst.new"
