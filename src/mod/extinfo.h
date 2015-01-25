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
    struct playerv1
    {
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
        union
        {
            uint8_t ia[sizeof(uint32_t)];
            uint32_t ui32;
        } ip;
        uint8_t reserved[4];
        uint32_t extflags;

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
            lilswap(&extflags);
        }

        playerv1() { memset(this, 0, sizeof(*this)); }

        static constexpr size_t basesize() { return sizeof(playerv1); }
        static constexpr int version() { return 1; }
    };

    static constexpr size_t align(size_t n, size_t a) { return n+n%a; }

    struct playerv2
    {
        struct /* base */
        {
            int cn;
            int ping;
            char name[align(MAXNAMELEN+1, 4)];
            char team[align(MAXTEAMLEN+1, 4)];
            int frags;
            int flags;
            int deaths;
            int teamkills;
            int acc;
            int health;
            int armour;
            int gunselect;
            int priv;
            int state;
            union
            {
                uint8_t ia[sizeof(uint32_t)];
                uint32_t ui32;
            } ip;
            uint8_t reserved[4];
        };

        struct /* extension */
        {
            bool ishopmod() { return mod == -2 || mod == -5; }
            bool isoomod() { return mod == -3; }
            bool ishopmodcompatible() { return ishopmod() || mod == -4; }

            int mod;
            int suicides;
            int shotdamage;
            int damage;
            int explosivedamage;
            int hits;
            int misses;
            int shots;
            int captured;
            int stolen;
            int defended;
        } ext;

        const char *getname() const
        {
            return *name ? name : "- empty name -";
        }

        void swapbase()
        {
            lilswap(&ping);
            lilswap(&frags);
            lilswap(&deaths);
            lilswap(&teamkills);
            lilswap(&acc);
            lilswap(&health);
            lilswap(&armour);
            lilswap(&ip.ui32);
        }

        playerv2() { memset(this, 0, sizeof(*this)); }

        playerv2(const playerv1 &v1)
        {
            cn = v1.cn;
            ping = v1.ping;
            copystring(name, v1.name, sizeof(name));
            copystring(team, v1.team, sizeof(team));
            frags = v1.frags;
            flags = v1.flags;
            deaths = v1.deaths;
            teamkills = v1.teamkills;
            acc = v1.acc;
            health = v1.health;
            armour = v1.armour;
            gunselect = v1.gunselect;
            priv = v1.priv;
            state = v1.state;
            ip.ui32 = v1.ip.ui32;
            zeroextbytes();
        }

        playerv2(void *base) { initbase(base); }

        void initbase(void *base)
        {
            memcpy(this, base, basesize());
            zeroextbytes();
        }

        void zeroextbytes() { memset((uchar*)this+basesize(), 0, extsize());  }

        #if CLANG_VERSION_AT_LEAST(3, 1, 0) && !CLANG_VERSION_AT_LEAST(3, 2, 0)
        // probably a buggy clang 3.1 warning, just silence it.
        // the assertion below would fail if it wouldn't be correct.
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Winvalid-offsetof"
        #endif

        static constexpr size_t basesize() { return __builtin_offsetof(playerv2, ext.mod); }
        static constexpr size_t extsize() { return sizeof(playerv2)-basesize(); }
        static constexpr int version() { return 2; }

        #if CLANG_VERSION_AT_LEAST(3, 1, 0) && !CLANG_VERSION_AT_LEAST(3, 2, 0)
        #pragma clang diagnostic pop
        #endif
    };

    static_assert(playerv1::basesize() == 56, "");
    static_assert(playerv2::basesize() == 80, "");

    struct extteaminfo
    {
        char team[MAXTEAMLEN+1];
        int score;
    };

    struct playerinfo
    {
        const char *country;
        const char *countrycode;
        extinfo::playerv2 ep;

        playerinfo(extinfo::playerv2 *e) : ep(*e)
        {
            country = mod::geoip::country(e->ip.ui32);
            countrycode = mod::geoip::countrycode(e->ip.ui32);
        }
    };

    typedef void (*callback)(int type, void *p, ENetAddress &addr);

    void startup();
    void shutdown();
    void slice();
    void connect();
    void newplayer(int cn);
    int getserveruptime();
    const char *getservermodname();
    void extinfoupdateevent(playerv2 &cp);
    void requestplayers(ENetAddress &addr);
    void requestteaminfo(ENetAddress &addr);
    void requestuptime(ENetAddress &addr);
    void addrecvcallback(callback cb);
    void delrecvcallback(callback cb);
} // namespace extinfo
} // namespace mod

#endif //EXTINFO_H
