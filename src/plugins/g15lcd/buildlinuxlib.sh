#!/bin/bash
COMPILER=$CXX
if [ "$COMPILER" == "" ]; then
  which "clang++" 1>/dev/null 2>1
  if [ $? -eq 0 ]; then
     COMPILER="clang++"
  else
    which "g++" 1>/dev/null 2>1
    if [ $? -eq 0 ]; then
      COMPILER="g++"
    else
      exit 1
    fi
  fi
fi
outdir=lib/linux
if [ `uname -m` == "x86_64" ]; then
  outdir=lib64/linux
fi
$COMPILER -O3 -fPIC -fno-exceptions -Wall -Wno-unused -Iinclude -c g15sdkwrapper_linux.cpp -o g15sdkwrapper.o || exit 1
strip --strip-unneeded g15sdkwrapper.o || exit 1
ar rcs $outdir/g15sdkwrapper.a g15sdkwrapper.o || exit 1
rm g15sdkwrapper.o || exit 1
echo ok
