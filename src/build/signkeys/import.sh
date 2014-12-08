#!/usr/bin/env bash

pushd "${0%/*}" &>/dev/null

gpg --import *.key
