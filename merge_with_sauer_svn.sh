#!/usr/bin/env bash
export LC_ALL=C
set -e

svnrepo="svn://svn.code.sf.net/p/sauerbraten/code/"
oldrev=$(cat src/last_sauer_svn_rev.h | grep '#define SAUERSVNREV' | cut -d ' ' -f3)

echo "getting latest revision..."

newrev=$(svn info $svnrepo)
newrev=$(echo "$newrev" | awk '/Revision:/ { print $2 }')

echo "latest revision is $newrev"

if [ $oldrev -ge $newrev ]; then
    echo "revision $newrev is already merged"
    exit 1
fi

set +e
git diff --quiet HEAD

if ! git diff-index --quiet HEAD --; then
    HAVE_CHANGES=1
    echo "stashing local changes..."
    git stash
else
    HAVE_CHANGES=0
fi

set -e
echo "merging..."
sh -c "svn diff -r$oldrev:$newrev $svnrepo >merge.patch"
SUCCESS=0
patch -m -p0 < merge.patch && rm merge.patch && {
    SUCCESS=1
}

echo "writing new revision to src/last_sauer_svn_rev.h"
sed -i "s/SAUERSVNREV $oldrev/SAUERSVNREV $newrev/g" src/last_sauer_svn_rev.h

if [ $SUCCESS -eq 1 ]; then
    echo "committing changes..."
    git commit -a -m "sync with sauer svn"

    if [ $HAVE_CHANGES -eq 1 ]; then
        echo "restoring local changes"
        git stash pop
    fi
fi