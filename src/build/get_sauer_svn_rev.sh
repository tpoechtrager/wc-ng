#!/usr/bin/env bash

pushd "${0%/*}" &>/dev/null

cat ../last_sauer_svn_rev.h | head -n 1 | awk '{print $3}'
