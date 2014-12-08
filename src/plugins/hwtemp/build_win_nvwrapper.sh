#!/bin/sh

BUILD="nvapi_wrapper_windows.cpp -Wall -shared -DBUILD_DLL -O3 -g0 -static-libgcc -static-libstdc++"

x86_64-w64-mingw32-g++ $BUILD -I include/ -L lib/win/x86_64 -l nvapi64 -o lib/win/x86_64/nvsdk.dll
i686-w64-mingw32-g++   $BUILD -I include/ -L lib/win/i686 -l nvapi -o lib/win/i686/nvsdk.dll

