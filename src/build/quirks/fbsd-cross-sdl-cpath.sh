#!/usr/bin/env bash

COMPILER=$1
[ $COMPILER == "ccache" ] && COMPILER=$2

echo -n "-I"
echo -n $(dirname $(echo '#include <string.h>' | $COMPILER -E - | grep '/usr/inc' | tail -n1 | awk '{print $3}' | tr '"' ' '))
echo -n "/../local/include/SDL2"
