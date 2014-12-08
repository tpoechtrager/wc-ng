#!/usr/bin/env bash

# prefer english warning/error messages
export LC_ALL="C"

# make sure we operate in the correct directory
pushd "${0%/*}" &>/dev/null

# use gnu make
MAKE=make
[[ `uname -s` == *BSD ]] && MAKE=gmake

# check syntax only

pushd .. &>/dev/null
$MAKE clean &>/dev/null
popd &>/dev/null

./build.sh CHECKSYNTAX=1 $@
