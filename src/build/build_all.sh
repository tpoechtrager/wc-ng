#!/usr/bin/env bash

pushd "${0%/*}" &>/dev/null

CC=clang CXX=clang++ ./package_linux32.sh
CC=clang CXX=clang++ ./package_linux64.sh

CC=gcc CXX=g++ PKGSUFFIX=_gcc ./package_linux32.sh
CC=gcc CXX=g++ PKGSUFFIX=_gcc ./package_linux64.sh

CC=icc CXX=icpc PKGSUFFIX=_icc ./package_linux32.sh
CC=icc CXX=icpc PKGSUFFIX=_icc ./package_linux64.sh

./package_freebsd64.sh
USEGCC=1 PKGSUFFIX=_gcc ./package_freebsd64.sh

./package_nsis_windows_clang.sh
PKGSUFFIX=_gcc ./package_nsis_windows_mingw.sh

./package_darwin.sh
USEGCC=1 PKGSUFFIX=_gcc ./package_darwin.sh
