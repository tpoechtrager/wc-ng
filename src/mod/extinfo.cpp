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

#include "game.h"

namespace game
{
    extern int showextinfo;
    extern int sessionid;
    extern bool fullyconnected;
}

namespace mod {
namespace extinfo
{
    struct exthost;
    exthost *eh = NULL;
    MODVARP(extinfopingrate, 1000, 2000, 10000);
    MODVARP(allowsettingextinfohost, 0, 1, 1);

    static inline ENetAddress getextinfoaddress()
    {
        ENetAddress exthost;

        if (allowsettingextinfohost && bouncerextinfohost.host)
            exthost = bouncerextinfohost;
        else if (allowsettingextinfohost && bouncerserverhost.host)
            exthost = bouncerserverhost;
        else
            exthost = curpeer->address;

        if (!allowsettingextinfohost || !bouncerextinfohost.host)
            exthost.port++;

        return exthost;
    }

    struct brokenserver
    {
        ENetAddress addr;
        int added;
        bool operator==(const ENetAddress &in) const { return addressequal(addr, in); }
        bool operator!=(const ENetAddress &in) const { return !(*this == in); }
        static constexpr int MAX_SERVERS = 1024;
        static constexpr int MAX_TIME_DIFF = 1000*60*60;
    };

    typedef brokenserver server;

    struct pingcolor : server
    {
        int lastaction = totalmillis;
        struct
        {
            int val;
            int validuntil;
            void newval()
            {
                volatile int *junk = new volatile int[5];
                int seedval = totalmillis;
                loopi(5) seedval ^= junk[i];
                delete[] junk;
                seedMT(seedval);
                do { val = randomMT(); } while (!val);
                validuntil = totalmillis+RAND_INTERVALL;
            }
            bool valid() { return totalmillis <= validuntil; }
            int get() { return val; }
        } vals[2]{};

        void update()
        {
            auto &cur = vals[0];
            auto &old = vals[1];
            if (!cur.valid())
            {
                old = cur;
                old.validuntil = totalmillis+OLD_VALID_MILLIS;
                cur.newval();
            }
            lastaction = totalmillis;
        }
        int get()
        {
            update();
            auto &cur = vals[0];
            return cur.get();
        }
        bool check(const int pingcolor)
        {
            update();
            for (auto &v : vals) if (v.valid() && v.get() == pingcolor) return true;
            return false;
        }
        bool inactive() { return totalmillis-lastaction <= MAX_INACTIVITY; }

        pingcolor(ENetAddress addr, int added) : server({addr, added}) {}
        static constexpr int RAND_INTERVALL = 10000;
        static constexpr int OLD_VALID_MILLIS = 5000;
        static constexpr int MAX_INACTIVITY = 10*60*1000;
        static constexpr int MAX_SERVERS = 10000;
    };

    struct exthost
    {
        int lastextping;
        int lastextpong;
        int lastextupdate;
        int lastplayerrecv;
        int serveruptime;
        const char *servermodname;
        int serveruptimerecv;
        int brokenpackets;
        ENetAddress addr;
        ENetSocket extsock;

        vector<brokenserver> brokenservers;
        vector<pingcolor> pingcolors;
        vector<callback> recvcallbacks;

        bool addbrokenserver(const ENetAddress &addr)
        {
            if (brokenservers.find(addr) >= 0) return false;
            if (brokenservers.length() >= brokenserver::MAX_SERVERS) brokenservers.remove(0);
            brokenservers.add({addr, totalmillis});
            return true;
        }

        pingcolor &getpingcolor(const ENetAddress &addr)
        {
            int i = pingcolors.find(addr);
            if (i>=0) return pingcolors[i];
            if (pingcolors.length() >= pingcolor::MAX_SERVERS) pingcolors.remove(0);
            pingcolors.add({addr, totalmillis});
            return pingcolors.last();
        }

        bool validatepacket(const ENetAddress &addr, const int type, ucharbuf &p,
                            const bool ignorerequest = false, const bool checknoerror = true)
        {
            int version;

            if (!ignorerequest)
            {
                getint(p); // request
                if (type == EXT_PLAYERSTATS && getint(p) != EXT_EXTENDED_PLAYERSTATS) goto error;
            }
            if (!getpingcolor(addr).check(getint(p))) goto error;
            if (getint(p) != EXT_ACK) goto error;

            version = getint(p);
            if (version < EXT_VERSION_MIN || version > EXT_VERSION) goto error;

            if (checknoerror && getint(p) != EXT_NO_ERROR) goto error;

            return true;

            error:;
            brokenpackets++;
            return false;
        }

        void extinforeceive()
        {
            uchar recvbuf[MAXTRANS];

            ENetBuffer buf;
            buf.data = recvbuf;
            buf.dataLength = sizeof(recvbuf);
            ENetAddress addr; int len;

            while ((len = enet_socket_receive(extsock, &addr, &buf, 1)) > 0)
            {
                ENetAddress curpeeraddress;
                ENetAddress bouncerhostaddress;

                if (curpeer)
                {
                    curpeeraddress = curpeer->address;
                    curpeeraddress.port++;
                }

                bouncerhostaddress = bouncerserverhost;
                bouncerhostaddress.port++;

                bool forcecallback = (!curpeer || ( !addressequal(addr, curpeeraddress) &&
                                                    !addressequal(addr, bouncerhostaddress) &&
                                                    !addressequal(addr, bouncerextinfohost) ));

                lastextpong = totalmillis;
                ucharbuf p((uchar*)buf.data, len);

                if (getint(p) != 0)
                {
                    brokenpackets++;
                    continue;
                }

                int type = getint(p);
                int origtype = type;

                if (type >= 100+EXT_UPTIME && type <= 100+EXT_TEAMSCORE)
                    type -= 100;

                switch (type)
                {
                    case EXT_PLAYERSTATS:
                    {
                        if (!validatepacket(addr, type, p)) break;

                        if (!p.remaining())
                        {
                            if (addbrokenserver(addr)) requestplayer(-1, &addr);
                            break;
                        }

                        switch (int type = getint(p))
                        {
                            case EXT_PLAYERSTATS_RESP_STATS:
                            {
                                playerv2 cp;

                                cp.cn = getint(p);
                                cp.ping = getint(p);

                                getstring(cp.name, p, sizeof(cp.name));
                                getstring(cp.team, p, sizeof(cp.team));
                                filtertext(cp.name, cp.name, false, false, sizeof(cp.name));
                                filtertext(cp.team, cp.team, false, false, sizeof(cp.team));

                                cp.frags = getint(p);
                                cp.flags = getint(p);
                                cp.deaths = getint(p);
                                cp.teamkills = getint(p);
                                cp.acc = getint(p);
                                cp.health = getint(p);
                                cp.armour = getint(p);
                                cp.gunselect = getint(p);
                                cp.priv = getint(p);
                                cp.state = getint(p);
                                cp.ip.ui32 = uint32_t(-1);

                                uint ip = 0;
                                p.get((uchar*)&ip, 3);

                                if (p.overread()) break;

                                if (ip != 0)
                                    cp.ip.ui32 = ip;

                                if (p.remaining())
                                {
                                    getint(p);
                                    int mod = getint(p);

                                    switch (mod)
                                    {
                                        case SM_HOPMOD:
                                        case SM_SUCKERSERV:
                                        case SM_ZEROMOD:
                                        {
                                            cp.ext.suicides = getint(p);
                                            cp.ext.shotdamage = getint(p);
                                            cp.ext.damage = getint(p);
                                            cp.ext.explosivedamage = getint(p);
                                            cp.ext.hits = getint(p);
                                            cp.ext.misses = getint(p);
                                            cp.ext.shots = getint(p);

                                            if (mod == SM_ZEROMOD && p.remaining() >= 2)
                                            {
                                                char tmp[2];
                                                p.get((uchar*)&tmp, 2);
                                                if (tmp[0] && tmp[1])
                                                {
                                                    bool iscontinent = islower(tmp[0]);
                                                    static_assert(sizeof(cp.ext.countrycode) >= sizeof(tmp), "");
                                                    memcpy(cp.ext.countrycode, tmp, sizeof(tmp));
                                                    // needed for demos
                                                    cp.dataflags |= (iscontinent ? playerv2::CONTINENT : playerv2::COUNTRYCODE);
                                                    static_assert(sizeof(cp.data) >= 2, "");
                                                    memcpy(&cp.data, tmp, 2);
                                                }
                                            }

#if 0
                                            conoutf("%s (%d): suicides: %d  shotdamage: %d  damage: %d  explosivedamage: %d  "
                                                    "hits: %d  misses: %d  shots: %d", cp.name, cp.cn, cp.ext.suicides, cp.ext.shotdamage,
                                                    cp.ext.damage, cp.ext.explosivedamage, cp.ext.hits, cp.ext.misses, cp.ext.shots);
#endif

                                            break;
                                        }

                                        case SM_OOMOD:
                                        {
                                            if (getint(p) != 1) break; // version
                                            cp.ext.suicides = getint(p);
                                            cp.ext.shotdamage = getint(p);
                                            cp.ext.damage = getint(p);
                                            cp.ext.captured = getint(p);
                                            cp.ext.stolen = getint(p);
                                            cp.ext.defended = getint(p);
                                            break;
                                        }

                                        default: goto error;
                                    }

                                    assign(cp.ext.mod, mod);

                                    if (p.overread())
                                    {
                                        error:;
                                        cp.ext.mod = SM_INVALID;
                                    }
                                }

                                loopv(recvcallbacks) recvcallbacks[i](type, &cp, addr);

                                if (forcecallback) break;

                                fpsent *d = game::getclient(cp.cn);

                                if (d)
                                {
                                    if (!d->extinfo)
                                        d->extinfo = new playerv2;

                                    *d->extinfo = cp;
                                    bool hadcountry = !!d->country;

                                    if (!d->country)
                                        geoip::lookupcountry(d->extinfo, d->country, d->countrycode);

                                    if (!hadcountry && d->country)
                                        event::run(event::EXTINFO_COUNTRY_UPDATE, "dss",
                                                   d->clientnum, d->country ? d->country : "??",
                                                   d->countrycode ? d->countrycode : "??");

                                    if (demorecorder::demorecord && demorecorder::clientdemorecordextinfo)
                                        demorecorder::self::putextinfoobj(&cp);

                                    extinfoupdateevent(cp);
                                    game::hasextinfo = true; // tell the client we have got extinfo informations from this server
                                }
                                break;
                            }

                            case EXT_PLAYERSTATS_RESP_IDS:
                            {
                                static vector<int> cns(32);
                                while (p.remaining()) cns.add(getint(p));
                                if (cns.length() > MAXCLIENTS*2) cns.setsize(MAXCLIENTS*2);
                                cns.add(-1);
                                loopv(recvcallbacks) recvcallbacks[i](type, cns.getbuf(), addr);
                                cns.pop();
                                if (forcecallback)
                                {
                                    cns.setsize(0);
                                    return;
                                }
                                lastextupdate = totalmillis;
                                lastplayerrecv = totalmillis;
                                cns.setsize(0);
                                break;
                            }
                        }
                        break;
                    }

                    case EXT_UPTIME:
                    {
                        if (p.get() != 1 || !validatepacket(addr, type, p, true, false)) break;
                        if (!p.remaining())
                        {
                            if (addbrokenserver(addr)) requestuptime(&addr);
                            break;
                        }
                        int uptime = getint(p);
                        int servermod = 0;
                        const char *modname = NULL;
                        if (p.remaining())
                        {
                            servermod = getint(p);
                            if (origtype == 100+EXT_UPTIME && servermod == -2) servermod = -7;
                            uint i = (servermod+2)*-1;
                            if (i < sizeofarray(SERVERMODNAMES)) modname = SERVERMODNAMES[i];
                        }
                        void *p[3] = { &uptime, &servermod, (void*)modname };
                        loopv(recvcallbacks) recvcallbacks[i](type, &p, addr);
                        if (forcecallback) break;
                        if (modname)
                        {
                            event::run(event::SERVER_MOD, "s", modname);
                            servermodname = modname;
                        }
                        serveruptime = uptime;
                        serveruptimerecv = totalmillis;
                        if (serveruptime < 0) serveruptime = -1;
                        break;
                    }

                    case EXT_TEAMSCORE:
                    {
                        if (!validatepacket(addr, type, p, true, false)) break;
                        bool teammode = getint(p) == 0; // teammode
                        getint(p); // gamemode
                        getint(p); // timeleft

                        if (!teammode) break;

                        loopv(recvcallbacks) recvcallbacks[i](type, NULL, addr);

                        do
                        {
                            extteaminfo ti;
                            getstring(ti.team, p, sizeof(ti.team));
                            ti.score = getint(p);
                            int extra = getint(p);
                            while (extra-- > 0 && !p.overread()) getint(p);

                            if (p.overread()) break;
                            loopv(recvcallbacks) recvcallbacks[i](type, &ti, addr);
                        } while(p.remaining());

                        if (forcecallback) break;
                        break;
                    }
                }
            }
        }

        void sendbuf(ucharbuf &p, ENetAddress *addr)
        {
            ENetAddress address;

            if (!addr)
            {
                if (!game::showextinfo) return;
                address = curpeer->address;
                address.port++;
            }
            else address = *addr;

            putint(p, getpingcolor(address).get());

            ENetBuffer buf;
            buf.data = p.buf;
            buf.dataLength = p.length();

            if (buf.dataLength >= 2) loopvrev(brokenservers)
            {
                const brokenserver bs = brokenservers[i];
                if (totalmillis-bs.added > brokenserver::MAX_TIME_DIFF) brokenservers.remove(i);
                if (bs != address) continue;
                ((uchar*)buf.data)[1] += 100u;
                break;
            }

            if (addressequal(*addr, bouncerextinfohost))
            {
                // include sessionid when sending request to bouncer forward socket

                packetbuf p(min(5+buf.dataLength, (size_t)MAXTRANS));

                putint(p, game::sessionid);
                p.put((uchar*)buf.data, buf.dataLength);

                buf.data = p.buf;
                buf.dataLength = p.length();

                enet_socket_send(extsock, &address, &buf, 1);
                return;
            }

            enet_socket_send(extsock, &address, &buf, 1);
        }

        void requestplayer(int cn, ENetAddress *addr = NULL)
        {
            uchar ubuf[100];
            ucharbuf p(ubuf, sizeof(ubuf));

            putint(p, 0);
            putint(p, EXT_PLAYERSTATS);
            putint(p, cn);
            putint(p, EXT_EXTENDED_PLAYERSTATS);

            sendbuf(p, addr);
        }

        void requestuptime(ENetAddress *addr = NULL)
        {
            uchar ubuf[100];
            ucharbuf p(ubuf, sizeof(ubuf));

            putint(p, 0);
            putint(p, EXT_UPTIME);
            p.put(1); // request server mod (extension)

            sendbuf(p, addr);
        }

        void requestteaminfo(ENetAddress *addr = NULL)
        {
            uchar ubuf[100];
            ucharbuf p(ubuf, sizeof(ubuf));

            putint(p, 0);
            putint(p, EXT_TEAMSCORE);

            sendbuf(p, addr);
        }

        void extprocess()
        {
            if (curpeer && game::fullyconnected)
            {
                bool a, b, c;
                a = !lastextpong || totalmillis - lastextpong >= extinfopingrate;
                b = !lastextping || totalmillis - lastextping >= extinfopingrate;
                c = totalmillis - lastextping >= extinfopingrate*2;

                if ((a && b) || c)
                {
                    ENetAddress extaddress = getextinfoaddress();
                    requestplayer(-1, &extaddress);
                    lastextping = totalmillis;
                }
            }

            {
                static int last = totalmillis;

                if (totalmillis-last >= 10000)
                {
                    loopvrev(pingcolors) if (pingcolors[i].inactive()) pingcolors.remove(i);
                    last = totalmillis;
                }
            }

            if (curpeer || !recvcallbacks.empty()) extinforeceive();
        }

        void connect()
        {
            serveruptime = -1;
            serveruptimerecv = -1;
            servermodname = NULL;

            ENetAddress extaddress = getextinfoaddress();
            requestuptime(&extaddress);
        }

        void createsocket()
        {
            extsock = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM);
            if (extsock == ENET_SOCKET_NULL) abort();
            enet_socket_set_option(extsock, ENET_SOCKOPT_NONBLOCK, 1);
        }

        void rebindpingport()
        {
            ENetSocket oldextsock = extsock;
            createsocket();
            enet_socket_destroy(oldextsock);
        }

        exthost() : lastextping(0), lastextpong(0), lastextupdate(0), lastplayerrecv(0), brokenpackets(0)
        {
            createsocket();
        }

        ~exthost() { enet_socket_destroy(extsock); }
    };

    void startup() { eh = new exthost; }
    void shutdown() { DELETEP(eh); }
    void slice() { if (eh) eh->extprocess(); }
    void connect() { if (curpeer && eh) eh->connect(); }
    void rebindpingport() { if (eh) eh->rebindpingport(); }
    void newplayer(int cn) { if (curpeer && eh && game::hasextinfo) eh->requestplayer(cn, &curpeer->address); }
    void requestplayers(ENetAddress &addr) { if (eh) eh->requestplayer(-1, &addr); }
    void requestteaminfo(ENetAddress &addr) { if (eh) eh->requestteaminfo(&addr); }
    void requestuptime(ENetAddress &addr) { if (eh) eh->requestuptime(&addr); }
    void addrecvcallback(callback cb) { if (eh) eh->recvcallbacks.add(cb); }
    void delrecvcallback(callback cb) { if (eh) eh->recvcallbacks.removeobj(cb); }
    int getserveruptime()
    {
        if (!curpeer || !eh || eh->serveruptimerecv < 0)
            return 0;

        if (eh->serveruptime <= 1) // protect ourself against extinfo protocol violation
            return 0;

        return eh->serveruptime+((totalmillis-eh->serveruptimerecv)/1000);
    }
    const char *getservermodname()
    {
        if (!curpeer || !eh)
            return NULL;

        return eh->servermodname;
    }

    void extinfoupdateevent(playerv2 &cp)
    {
        const char *eventargs;

#ifdef ENABLE_IPS
        eventargs = "udssdddddddddduuuu";
#else
        eventargs = "udssdddddddddd";
#endif

        event::run(event::EXTINFO_UPDATE, eventargs,
                   cp.cn, cp.ping, cp.name, cp.team, cp.frags, cp.flags, cp.deaths, cp.teamkills,
                   cp.acc, cp.health, cp.armour, cp.gunselect, cp.priv, cp.state,
                   cp.ip.ia[0], cp.ip.ia[1], cp.ip.ia[2], cp.ip.ia[3]);
    }

    bool havecountrynames(int type, void *pplayers)
    {
        switch (type)
        {
            case EXTINFO_COUNTRY_NAMES_SCOREBOARD:
            {
                bool rv = false;
                game::clients.add(game::player1);
                loopv(game::clients)
                {
                    fpsent *c = game::clients[i];
                    if (c && c->country)
                    {
                        rv = true;
                        break;
                    }
                }
                game::clients.pop();
                return rv;
            }

            case EXTINFO_COUNTRY_NAMES_SERVERBROWSER:
            {
                vector<extinfo::playerinfo> &players = *(vector<extinfo::playerinfo>*)pplayers;
                loopv(players) if (players[i].country) return true;
                break;
            }
        }

        return false;
    }

} // namespace extinfo
} // namespace mod
