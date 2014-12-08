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

#include <cube.h>
#include <openssl/crypto.h>

namespace mod {
namespace crypto {

static sdlmutex *cryptomutex = NULL;

void init(bool threadsafe)
{
    if (!threadsafe) return;
    cryptomutex = new sdlmutex[CRYPTO_num_locks()];
    auto threadid = []() -> unsigned long { return SDL_ThreadID(); };
    auto cryptolock = [](int mode, int n, const char *, int)
    {
        if (mode&CRYPTO_LOCK) cryptomutex[n].lock();
        else cryptomutex[n].unlock();
    };
    CRYPTO_set_id_callback(threadid);
    CRYPTO_set_locking_callback(cryptolock);
}

void deinit()
{
    CRYPTO_set_id_callback(NULL);
    CRYPTO_set_locking_callback(NULL);
    DELETEA(cryptomutex);
}

} // namespace crypto
} // namespace mod