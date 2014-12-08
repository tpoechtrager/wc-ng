#!/usr/bin/env bash

pushd "${0%/*}" &>/dev/null

find .. -name "*.gcda" -exec rm -f {} \;
