#!/usr/bin/env bash

pushd "${0%/*}" &>/dev/null

. include/common.inc.sh

prog="cpucount${EXESUFFIX}"

test -f "${prog}" || cc cpucount.c -o $prog || exit 1

eval "./${prog}"
