/***********************************************************************
 *  WC-NG - Cube 2: Sauerbraten Modification                           *
 *  Copyright (C) 2019 by Thomas Poechtrager                           *
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


#include "game.h"

namespace mod {
namespace neutral_player_names {

void onchange()
{
    extern int neutralplayernames;

    if (neutralplayernames)
    {
        loopv(game::clients)
        {
            fpsent *d = game::clients[i];
            if (!d || d->clientnum >= 128 || !d->origname[0]) continue;
            copystring(d->origname, d->origname);
            newname(d->name, MAXNAMELEN);
        }
    }
    else
    {
        loopv(game::clients)
        {
            fpsent *d = game::clients[i];
            if (!d || d->clientnum >= 128 || !d->origname[0]) continue;
            copystring(d->name, d->origname);
        }
    }
}

MODVARFP(neutralplayernames, 0, 0, 1, onchange());


static constexpr const char *PLAYERNAMES[] = 
{
    "Lynn", "Essie", "Karl", "Oscar", "Howard", "Brian", "Ruben", "Jay", "Marc", "Ross", "James", "Bob",
    "Gordon", "Glen", "Jon", "Irving", "Darrel", "Matthew", "Jeremy", "Darin", "Jerald", "Juan", "Cary", "Seth",
    "Kent", "Orlando", "Morris", "Daryl", "Leroy", "Bennie", "Dale", "Gilbert", "Marvin", "Shane", "Devin", "Byron",
    "Curtis", "Omar", "Nelson", "Marcos", "Earl", "Lane", "Gino", "Dee", "Reggie", "Gail", "Porter", "Genaro", "Robby",
    "Logan", "Aldo", "Scotty", "Scot", "Aldo", "Murray", "Rodrick", "Mervin", "Noe", "Wes", "Nolan", "Robbie", "Keneth",
    "Denny", "Monroe", "Theron", "Tanner"
};

void newname(char *name, int len)
{
    if (!neutralplayernames) return;

    vector<const char *> pool;
    loopi(sizeofarray(PLAYERNAMES)) pool.add(PLAYERNAMES[i]);

    loopv(game::clients)
    {
        fpsent *d = game::clients[i];
        if (!d || !d->name[0] || !d->origname[0]) continue;
        loopvj(pool) if (!strcmp(d->name, pool[j]))
        {
            pool.remove(j);
            break;
        }
    }

    if (pool.empty())
    {
        copystring(name, PLAYERNAMES[rnd(sizeofarray(PLAYERNAMES))], len);
        return;
    }

    copystring(name, pool[rnd(pool.length())], len);
}

const char *newname(const extinfo::playerv2 &ep)
{
    if (!neutralplayernames) return NULL;
    uint seed = ep.cn + ep.ip.ui32;
    loopi(strlen(ep.name)) seed += ep.name[i];
    return PLAYERNAMES[seed%sizeofarray(PLAYERNAMES)];
}

void replacenames(char *text, int len)
{
    if (!neutralplayernames) return;

    strtool tmp = text;
    strtool newtext;
    strtool *strs = NULL;

    strtool_size_t n = tmp.split(" ", &strs);
    if (!n) return;

    loopai(n)
    {
        strtool &str = strs[i];
        tmp = str;
        filtertext(tmp, false, false);

        size_t orignamelen;
        int c;

        loopvj(game::clients)
        {
            fpsent *d = game::clients[j];
            if (!d || !d->name[0] || !d->origname[0]) continue;
            orignamelen = strlen(d->origname);
            if (tmp.compare(d->origname, orignamelen)) continue;
            if (tmp.length() == orignamelen) goto match;
            c = tmp[orignamelen];
            if (c == ':' || c == ',' || c == '!' || c == '(') goto match;
            continue;
            match:;
            str.replace(d->origname, d->name);
        }
    }

    tmp.clear();
    tmp.join((const strtool **)&strs, n, " ");
    copystring(text, tmp.str(), len);

    delete[] strs;
}

}
}