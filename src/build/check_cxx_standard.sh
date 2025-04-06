#!/usr/bin/env bash

export LC_ALL=C
COMPILER=$1

check()
{
    error=0
    case `echo "void*p=nullptr;auto x=[](){};" | $COMPILER -xc++ -std=$1 -S -o- - 2>&1 1>/dev/null` in
        *warning*|*error*|*invalid* ) error=1 ;;
    esac
    test $? -eq 0 && test $error -eq 0 && echo "-std=$1" && exit 0
}

which $1 &>/dev/null || exit 1

#check "gnu++1z"
check "gnu++14"
check "gnu++1y"
check "gnu++11"
check "gnu++0x"

#check "c++1z"
check "c++14"
check "c++1y"
check "c++11"
check "c++0x"

exit 1
