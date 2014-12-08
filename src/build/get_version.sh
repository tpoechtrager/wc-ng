#!/usr/bin/env bash

set -e

pushd ${0%/*} &>/dev/null

. include/common.inc.sh

./get_revision.sh

prog="wcversion${EXESUFFIX}"

c++ -fno-exceptions get_version.cpp -o wcversion

./$prog
