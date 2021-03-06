#ifndef __CUBE_H__
#define __CUBE_H__

#define _FILE_OFFSET_BITS 64

#ifndef __GNUC__
#define _CRT_SECURE_NO_WARNINGS //NEW
#endif

#ifdef WIN32
#define _USE_MATH_DEFINES
#endif
//NEW
#ifdef __GNUC__
#include <stdint.h>
#endif
//NEW END
#include <math.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <limits.h>
#include <assert.h>
#include <time.h>

#ifdef WIN32
  #define WIN32_LEAN_AND_MEAN
  #ifdef _WIN32_WINNT
  #undef _WIN32_WINNT
  #endif
  #define _WIN32_WINNT 0x0500
  #include "windows.h"
  #ifndef _WINDOWS
    #define _WINDOWS
  #endif
  #ifndef __GNUC__
    #include <eh.h>
    #include <dbghelp.h>
    #include <intrin.h>
  #endif
  //#define ZLIB_DLL
#endif

#ifndef STANDALONE
  #ifdef __APPLE__
    #include "SDL.h"
    #define GL_GLEXT_LEGACY
    #define __glext_h_
    #include <OpenGL/gl.h>
  #else
    #include <SDL.h>
    #include <SDL_opengl.h>
  #endif
#endif

#include <enet/enet.h>

#include <zlib.h>

#include "tools.h"
#include "geom.h"
#include "ents.h"
#include "command.h"

#ifndef STANDALONE
#include "glexts.h"
#include "glemu.h"
#endif

#include "iengine.h"

//NEW
#include "compat.h"

#ifdef _MSC_VER
#include "stdint.h"
#include "inttypes.h"
#endif

#include "mod.h"

#if !defined(STANDALONE) || defined(PLUGIN)
#include "plugin.h"
#endif
//NEW END

#include "igame.h" //NEW - moved down

#endif

