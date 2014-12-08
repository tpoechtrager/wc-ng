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

#include <stddef.h>
#include <string.h>
#include <assert.h>

static inline void szeromemory(void *ptr, size_t size)
{
    if (!size) return;
    memset(ptr, '\0', size);
    *(volatile char*)ptr = *(volatile char*)ptr;
}

#if defined(HAVE_OPENSSL) && !defined(_WIN32) && !defined(__APPLE__)
#include <stdint.h>
#include <dlfcn.h>

extern "C" size_t malloc_usable_size(void *ptr);

static void free_hook(void *ptr, const void*)
{
    extern int useopenssl;
    if (useopenssl) szeromemory(ptr, malloc_usable_size(ptr));
}

__attribute__((constructor (101)))
__attribute__((visibility ("hidden")))
void init_free_hook()
{
   void *f = dlsym(NULL, "__free_hook");
   if (f) *(uintptr_t*)f = (uintptr_t)&free_hook;
}
#endif

template<class T, class C = bool, class L = size_t*>
struct sdealloc
{
    T &data;
    const C cond;
    const L length;

    sdealloc(T &data, C cond = C(1), L length = NULL) :
        data(data), cond(cond), length(length) {}

    ~sdealloc()
    {
        if (!cond) return;
        size_t len = sizeof(data);

        if (length)
        {
            len = *length;
            assert(len < sizeof(data));
        }

        szeromemory(&data, len);
    }
};

#define SDEALLOC(data, cond) \
  sdealloc<decltype(data), decltype(cond)> __secure_dealloc_##data(data, cond)
