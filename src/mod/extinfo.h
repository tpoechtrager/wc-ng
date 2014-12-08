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

#ifndef EXTINFO_H
#define EXTINFO_H

#define EXT_ACK                         -1
#define EXT_VERSION                     105
#define EXT_NO_ERROR                    0
#define EXT_ERROR                       1
#define EXT_PLAYERSTATS_RESP_IDS        -10
#define EXT_PLAYERSTATS_RESP_STATS      -11
#define EXT_UPTIME                      0
#define EXT_PLAYERSTATS                 1
#define EXT_TEAMSCORE                   2

#define EXT_VERSION_MIN                104

#ifndef MAXNAMELEN
#define MAXNAMELEN 15
#define MAXTEAMLEN 4
#endif

namespace mod {
namespace extinfo
{
    enum flags
    {
        FULL_PLAYER_IP = 1<<0, // unused
        HAS_SUICIDES   = 1<<1  // server demos include suicides
    };

    struct player
    {
        static const int VERSION = 1;

        uint8_t cn;
        int16_t ping;
        char name[MAXNAMELEN+1];
        char team[MAXTEAMLEN+1];
        int16_t frags;
        int8_t flags;
        int16_t deaths;
        int16_t teamkills;
        int16_t acc;
        int16_t health;
        int16_t armour;
        int8_t gunselect;
        int8_t priv;
        int8_t state;
        union {
            uint8_t ia[sizeof(uint32_t)];
            uint32_t ui32;
        } ip;
        uint16_t suicides;
        uint8_t reserved[2];
        uint32_t extflags;

        const char *getname() const
        {
            return *name ? name : "- empty name -";
        }

        void swap()
        {
            lilswap(&ping);
            lilswap(&frags);
            lilswap(&deaths);
            lilswap(&teamkills);
            lilswap(&acc);
            lilswap(&health);
            lilswap(&armour);
            lilswap(&ip.ui32);
            lilswap(&suicides);
            lilswap(&extflags);
        }

        player() { memset(this, 0, sizeof(*this)); }
    };

    struct extteaminfo
    {
        char team[MAXTEAMLEN+1];
        int score;
    };

    struct playerinfo
    {
        const char *country;
        const char *countrycode;
        extinfo::player ep;

        playerinfo(extinfo::player *e) : ep(*e)
        {
            country = mod::geoip::country(e->ip.ui32);
            countrycode = mod::geoip::countrycode(e->ip.ui32);
        }
    };

    typedef void (*callback)(int type, void *p, ENetAddress& addr);

    void startup();
    void shutdown();
    void slice();
    void connect();
    void newplayer(int cn);
    int getserveruptime();
    const char *getservermodname();
    void extinfoupdateevent(player& cp);
    void requestplayers(ENetAddress& addr);
    void requestteaminfo(ENetAddress& addr);
    void requestuptime(ENetAddress& addr);
    void addrecvcallback(callback cb);
    void delrecvcallback(callback cb);

    void freeextinfo(void **extinfo);
} // namespace extinfo
} // namespace mod

#endif //EXTINFO_H
