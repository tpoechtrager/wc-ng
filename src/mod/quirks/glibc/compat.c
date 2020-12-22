/***********************************************************************
 *  WC-NG - Cube 2: Sauerbraten Modification                           *
 *  Copyright (C) 2014, 2015, 2020 by Thomas Poechtrager               *
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <dlfcn.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus*/

#define STRINGIFY(x) #x
#define TOSTRING(x)  STRINGIFY(x)

#define INIT(FN, FN_TYPE) \
static FN_TYPE FN##_addr = NULL; \
__attribute__((constructor (101))) \
__attribute__((visibility ("hidden"))) \
void init___wrap_##FN(void) \
{ \
    if (!FN##_addr) \
    { \
        FN##_addr = (FN_TYPE)dlsym(NULL, TOSTRING(FN)); \
        if (!FN##_addr) abort(); \
    } \
}

#define WRAP1(FN, TYPE_RET, TYPE_ARG1) \
typedef TYPE_RET (*FN_TYPE_##FN)(TYPE_ARG1); \
INIT(FN, FN_TYPE_##FN) \
TYPE_RET __wrap_##FN(TYPE_ARG1 arg1) { return FN##_addr(arg1); }

#define WRAP2(FN, TYPE_RET, TYPE_ARG1, TYPE_ARG2) \
typedef TYPE_RET (*FN_TYPE_##FN)(TYPE_ARG1, TYPE_ARG2); \
INIT(FN, FN_TYPE_##FN) \
TYPE_RET __wrap_##FN(TYPE_ARG1 arg1, TYPE_ARG2 arg2) { return FN##_addr(arg1, arg2); }

#define WRAP3(FN, TYPE_RET, TYPE_ARG1, TYPE_ARG2, TYPE_ARG3) \
typedef TYPE_RET (*FN_TYPE_##FN)(TYPE_ARG1, TYPE_ARG2, TYPE_ARG3); \
INIT(FN, FN_TYPE_##FN) \
TYPE_RET __wrap_##FN(TYPE_ARG1 arg1, TYPE_ARG2 arg2, TYPE_ARG3 arg3) { return FN##_addr(arg1, arg2, arg3); }


/*
 * Remove glibc symbol versions.
 */

WRAP1(exp, double, double)
WRAP1(expf, float, float)

WRAP1(exp2, double, double)
WRAP1(exp2f, float, float)

WRAP1(log, double, double)
WRAP1(logf, float, float)

WRAP2(pow, double, double, double)
WRAP2(powf, float, float, float)

WRAP3(reallocarray, void *, void *, size_t, size_t);
WRAP3(pthread_sigmask, int, int, const void *, void *);

#ifdef __cplusplus
}
#endif /* __cplusplus */
