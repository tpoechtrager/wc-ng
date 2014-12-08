#!/usr/bin/env bash

pushd "${0%/*}" &>/dev/null

function trimtrailing()
{
    find $1 -type f \( -name '*.cpp' -o -name '*.c' -o -name '*.h' -o -name '*.sh' -o -name '*.css' -o -name '*.html' \) \
        -exec sed --in-place 's/[[:space:]]\+$//' {} \+
}

trimtrailing "../mod"
trimtrailing "../build"
trimtrailing "../../doc"
#trimtrailing "../plugins/"
