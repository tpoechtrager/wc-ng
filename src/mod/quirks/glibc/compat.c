/***********************************************************************
 *  WC-NG - Cube 2: Sauerbraten Modification                           *
 *  Copyright (C) 2014, 2015 by Thomas Poechtrager                     *
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

#define STRINGIFY(x) #x
#define TOSTRING(x)  STRINGIFY(x)

#if defined(__PIC__) && defined(__x86_64__)
#define JMP(ADDR)                                                              \
do {                                                                           \
    asm("jmp *%0" :: "r" (ADDR));                                              \
    __builtin_unreachable();                                                   \
} while (0)
#define JMPFN JMP
#else
#define JMP(SN)                                                                \
do {                                                                           \
    asm("jmp *" TOSTRING(SN));                                                 \
    __builtin_unreachable();                                                   \
} while (0)
#define JMPFN(SN)                                                              \
do {                                                                           \
    asm("jmp " TOSTRING(SN));                                                  \
    __builtin_unreachable();                                                   \
} while (0)
#endif /* __PIC__ && __x86_64__ */

#define LOOKUP(FP, SN, REQUIRED)                                               \
do {                                                                           \
    FP = (void (*)())dlsym(NULL, TOSTRING(SN));                                \
    if (FP) return;                                                            \
    if (REQUIRED)                                                              \
    {                                                                          \
        fprintf(stderr, "cannot find address of " TOSTRING(SN) "\n");          \
        asm("int $3");                                                         \
        return;                                                                \
    }                                                                          \
} while (0)

#define WRAPSYMBOL(OS, WS)                                                     \
void (*_##WS)(void);                                                           \
__attribute__((constructor (101)))                                             \
__attribute__((visibility ("hidden")))                                         \
void init___wrap_##OS(void)                                                    \
{                                                                              \
    LOOKUP(_##WS, WS, 1);                                                      \
}                                                                              \
void __wrap_##OS(void)                                                         \
{                                                                              \
    JMP(_##WS);                                                                \
}

#define WRAPFINITE(SN)   WRAPSYMBOL(__##SN##_finite, SN)
#define REMOVESYMVER(SN) WRAPSYMBOL(SN, SN)

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus*/

WRAPFINITE(exp)
WRAPFINITE(expf)
WRAPFINITE(log)
WRAPFINITE(logf)
WRAPFINITE(log10f)
WRAPFINITE(pow)
WRAPFINITE(powf)
WRAPFINITE(acosf)
WRAPFINITE(atan2f)
WRAPFINITE(fmod)
WRAPFINITE(acos)
WRAPFINITE(asin)
WRAPFINITE(log10)
WRAPFINITE(atan2)

REMOVESYMVER(reallocarray)

REMOVESYMVER(exp2f)

REMOVESYMVER(pow)
REMOVESYMVER(log)
REMOVESYMVER(exp)

REMOVESYMVER(powf)
REMOVESYMVER(logf)
REMOVESYMVER(expf)

void __wrap_memcpy(void)
{
    /* GLIBC 2.2.5 implemented memcpy() as memmove() */
    JMPFN(memmove);
}

extern void __chk_fail(void);

unsigned long int __wrap___fdelt_chk(unsigned long int index)
{
    if (index >= FD_SETSIZE)
        __chk_fail();

    return index / __NFDBITS;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
