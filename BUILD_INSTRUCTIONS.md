### Build Instructions ###

## Supported Compilers ##

Tier 1:

  * Clang 3.1+
  * GCC 4.7+

Tier 2:

  * Intel Compiler (linux)

Not Supported:

  * MSVC
  * MinGW32 (MinGW-w64 is supported)
  * MSYS + MinGW (may work though)

## Unix/Linux ##

The static way (recommended):

  * Pre-built static libraries are used, thus you don't need
    to install any dependency libraries.
  * `make USESTATICLIBS=1 [LTO=1 ARCH=i686 CC=.. CXX=...] -j...`

The non-static way: 

  * Install the required libraries:  ...
  * `make [LTO=1] -j...`

Package (static):

  * Linux:
      * i686: `./build/package_linux32.sh`
      * x86\_64: `./build/package_linux64.sh`
  * FreeBSD:
      * i686: `./build/package_freebsd32.sh`
      * amd64: `./build/package_freebsd64.sh`
  * Other:
      * [ARCH=...] `./build/package.sh`

## OS X ##

There is only a static way on OS X:

  * `[TARGETS="-m32 -m64"] make [UNIVERSAL=1 LTO=1 OBJCC=... OBJCXX=... CC=... CXX=...] -j...`

Cross compiling from Unix/Linux to OS X:

  * Install OSXCross
  * `[TARGETS="-m32 -m64"] make PLATFORM=Darwin [UNIVERSAL=1 LTO=1] -j...`

Package:

  * `./build/package_darwin.sh`

## Windows ##

Cross compiling from Unix/Linux to Windows:

  * MinGW-w64:
      * Install mingw-w64 (mingw32 is not supported)
      * i686: `make PLATFORM=MINGW ARCH=i686 -j... [LTO=1]`
      * x86_64: `make PLATFORM=MINGW ARCH=x86_64 -j... [LTO=1]`
  * (w)clang:
      * Install clang, mingw-w64 and wclang
      * i686: `make PLATFORM=MINGW-CLANG ARCH=i686 -j... [LTO=1]`
      * i686: `make PLATFORM=MINGW-CLANG ARCH=x86_64 -j... [LTO=1]`

Package:

  * MinGW:
      * i686: `./build/package_w32-mingw.sh`
      * x86\_64: `./build/package_w64-mingw.sh`
      * Installer (i686 & x86_64):
          * Install NSIS and unix2dos (Yes, NSIS is available for Unix/Linux)
          * `./build/package_nsis_windows_mingw.sh`
  * (w)clang:
      * i686: `./build/package_w32-clang.sh`
      * x86\_64: `./build/package_w64-clang.sh`
      * Installer (i686 & x86_64):
          * Install NSIS and unix2dos (Yes, NSIS is available for Unix/Linux)
          * `./build/package_clang_windows_mingw.sh`

