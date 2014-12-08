/***********************************************************************
 *  WC-NG - Cube 2: Sauerbraten Modification                           *
 *  Copyright (C) 2014 by Thomas Poechtrager                           *
 *  t.poechtrager@gmail.com                                            *
 *                                                                     *
 *  This program is free software; you can redistribute it and/or      *
 *  modify it under the terms of the GNU General Public License        *
 *  as published by the Free Software Foundation; either version 2     *
 *  of the License, or (at your option) any later version.             *
 *                                                                     *
 *  This program is distributed in the hope that it will be useful,    *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of     *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the      *
 *  GNU General Public License for more details.                       *
 *                                                                     *
 *  You should have received a copy of the GNU General Public License  *
 *  along with this program; if not, write to the Free Software        *
 *  Foundation, Inc.,                                                  *
 *  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.      *
 ***********************************************************************/

#ifdef _MSC_VER
static inline const char **getcompilerversion()
{
    static const char *compilerversion = NULL;

    if (!compilerversion)
    {
        switch (_MSC_VER)
        {
        case 1600: compilerversion = "2010 (10.0)"; break;
        case 1700: compilerversion = "2012 (11.0)"; break;
        case 1800: compilerversion = "2013 (12.0)"; break;
        default: compilerversion = "unknown"; break;
        }
    }

#ifdef __clang__
    static bool once = false;
    static string clang;
    if (!once) formatstring(clang)("%s (%s)", __clang_version__, compilerversion);
    static const char *compiler[] = { "clang", clang };
#else
    static const char *compiler[] = { "Visual C++", compilerversion };
#endif
    return compiler;
}
#define HAVE_COMPILER_VENDOR_STRING
#endif //_MSC_VER

#ifndef HAVE_COMPILER_VENDOR_STRING
#ifdef __clang__
static inline const char **getcompilerversion()
{
    static const char *compiler[] =  { "clang", __clang_version__ };
    return compiler;
}
#define HAVE_COMPILER_VENDOR_STRING
#endif
#endif

#ifndef HAVE_COMPILER_VENDOR_STRING
#ifdef __ICC
static inline const char **getcompilerversion()
{
    static const char *compiler[] =  { "Intel C++", __VERSION__ };
    return compiler;
}
#define HAVE_COMPILER_VENDOR_STRING
#endif
#endif

#ifndef HAVE_COMPILER_VENDOR_STRING
#ifdef __MINGW32__
static inline const char **getcompilerversion()
{
    static const char *compiler[] =  { "MinGW (GCC)", __VERSION__ };
    return compiler;
}
#define HAVE_COMPILER_VENDOR_STRING
#endif
#endif

#ifndef HAVE_COMPILER_VENDOR_STRING
#ifdef __GNUC__
static inline const char **getcompilerversion()
{
    static const char *compiler[] =  { "GCC", __VERSION__ };
    return compiler;
}
#define HAVE_COMPILER_VENDOR_STRING
#endif
#endif

#ifndef HAVE_COMPILER_VENDOR_STRING
static inline const char **getcompilerversion()
{
    static const char *compiler[] =  { "unknown", "unknown" };
    return compiler;
}
#define HAVE_COMPILER_VENDOR_STRING
#endif

#define GCC_VERSION_AT_LEAST(major, minor, patch) \
    (defined(__GNUC__) && \
    (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__) >= \
    (major * 10000 + minor * 100 + patch))

#define CLANG_VERSION_AT_LEAST(major, minor, patch) \
    (defined(__clang__) && !defined(__apple_build_version__) && \
    (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__) >= \
    (major * 10000 + minor * 100 + patch))

#define APPLE_CLANG_VERSION_AT_LEAST(major, minor, patch) \
    (defined(__clang__) && defined(__apple_build_version__) && \
    (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__) >= \
    (major * 10000 + minor * 100 + patch))

#define ICC_VERSION_AT_LEAST(major, minor, patch) \
    (defined(__INTEL_COMPILER) && \
    ((__INTEL_COMPILER * 10000 + __INTEL_COMPILER_UPDATE) >= \
    ((major * 100 + minor) * 10000) + patch))

#if __cplusplus >= 201103
#define CXX11
#endif // C++11

#if __cplusplus > 201103
#define CXX14
#endif // C++14

#ifdef __MINGW32__
#ifndef __x86_64__
#if GCC_VERSION_AT_LEAST(4, 9, 0)
#ifdef __OPTIMIZE__
#define IS_BROKEN_MINGW49
#endif // __OPTIMIZE__
#endif // GCC >= 4.9.0
#endif // ! __x86_64__
#endif // __MINW32__
