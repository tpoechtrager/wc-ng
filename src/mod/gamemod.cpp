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

#include "game.h"

using namespace game;
using namespace mod;

namespace game
{
    extern int scaletimeleft;
}

namespace gamemod
{
    MODVARP(autofollowactioncn, 0, 0, 1);
    MODVARP(actioncnfollowtime, 500, 5000, 60000);

    void followactioncn()
    {
        static int last = 0;

        if (!autofollowactioncn || player1->state != CS_SPECTATOR)
            return;

        fpsent *d;

        if (totalmillis-last < actioncnfollowtime &&
           (d = getclient(following)) && d->state == CS_ALIVE)
            return;

        int cn = getactioncn();

        if (cn < 0)
            return;

        last = totalmillis;

        following = cn;
    }

    const char *calcteamkills(void *c, char *buf, size_t len, bool scoreboard, bool increase)
    {
        fpsent *d = (fpsent*)c;
        auto *ep = d->extinfo;

        if (!ep)
            return NULL;

        auto &teamkills = ep->teamkills;

        if (increase)
        {
            teamkills++;
            ep->frags--;
        }

        strtool str(buf, len);
        str.fmt(scoreboard ? "%d" : "%dx", teamkills);
        return str.getbuf();
    }

    const char *countteamkills(void *actor, bool increase, char *buf, size_t len)
    {
        if (!actor)
            return NULL;

        return calcteamkills(actor, buf, len, false, increase);
    }

    static const char *formatstats(const char *fmt, const char *fmtstart, mod::strtool &str,
                                   fpsent *d, mod::strtool &error, const char *currentmode = NULL,
                                   bool notmatching = false);

    static bool formatplayerstats(int type, fpsent *d, extinfo::playerv2 *ep, mod::strtool &str)
    {
        switch (type)
        {
            case 'n':
                str << d->name;
                break;

            case 't':
                str << d->team;
                break;

            case 'i':
                str << (ep ? ep->teamkills : d->teamkills);
                break;

            case 'f':
                str << d->frags;
                break;

            case 'd':
                str << (d==player1 ? max(d->deaths, ep ? ep->deaths : 0) : ep ? ep->deaths : 0);
                break;

            case 'k':
            {
                int deaths = ep ? ep->deaths : d->deaths;
                str.fmt("%.2f", float(d->frags)/float(deaths ? deaths:1));
                break;
            }

            case 'a':
            {
                // always use extinfo if possible, can't really use max() on this one
                int acc = ep ? ep->acc : (d==player1 ? (d->totaldamage*100)/max(d->totalshots, 1) : 0);
                str << acc;
                break;
            }

            case 'h':
            {
                if (d == player1)
                {
                    str.fmt("%.2f", d->highresping);
                    break;
                }
            }

            case 'p':
                str << d->ping;
                break;

            case 's':
            {
                const char *state;
                switch (d->state)
                {
                    case CS_ALIVE: state = "alive"; break;
                    case CS_DEAD: state = "dead"; break;
                    case CS_SPAWNING: state = "spawning"; break;
                    case CS_EDITING: state = "editing"; break;
                    case CS_SPECTATOR: state = "spectating"; break;
                    default: state = "unknown";
                }
                str << state;
                break;
            }

            case 'x':
                str << d->flags;
                break;

            default:
                return false;
        }

        return true;
    }

    static bool formatnonplayerstats(int type, mod::strtool &str)
    {
        switch (type)
        {
            case 'u':
                str << game::gamespeed;
                break;

            case 'g':
                str << gamemodes[clamp(gamemode-STARTGAMEMODE, 0, NUMGAMEMODES)].name;
                break;

            case 'l':
                str << (game::gamepaused ? "paused " : "");
                break;

            case 'o':
            {
                const char *mapinfo = getmapinfo();
                str << (mapinfo ? mapinfo : "");
                break;
            }

            case 'm':
                str << getclientmap();
                break;

            case 'e':
                str << server::mastermodename(mastermode);
                break;

            case 'y':
            case 'z':
            {
                int timeleft, min, sec;

                if (!m_edit)
                {
                    timeleft = (intermission ? 0 : max(maplimit-lastmillis, 0)/1000);
                    timeleft *= 100/(scaletimeleft ? game::gamespeed : 100);
                }
                else timeleft = lastmillis/1000;

                min = timeleft/60, sec = timeleft%60;

                if (type == 'z')
                {
                    const char *cc = "\f7";
                    if (!m_edit)
                    {
                        if (min == 0)
                        {
                            if (sec < 30/* && lastmillis%1000 >= 500*/) cc = "\f3";
                            else cc = "\f4";
                        }
                    }
                    str.fmt("\fs%s%d:%-2.2d\fr", cc, min, sec);
                    break;
                }

                str.fmt("%d:%-2.2d", min, sec);
                break;
            }

            default:
                return false;
        }

         return true;
    }

    static bool formatbandwidthstats(int type, mod::strtool &str)
    {
        switch (type)
        {
        case 'w':
            str.fmt("%.2f", ((clientbwstats*)curpeer->bandwidthStats.data)->curincomingkbs);
            break;

        case 'q':
            str.fmt("%.2f", ((clientbwstats*)curpeer->bandwidthStats.data)->curoutgoingkbs);
            break;

        case 'r':
            str.fmt("%d", ((clientbwstats*)curpeer->bandwidthStats.data)->curincomingpacketssec);
            break;

        case 'c':
            str.fmt("%d", ((clientbwstats*)curpeer->bandwidthStats.data)->curoutgoingpacketssec);
            break;

        case 'b':
            str.fmt("%.2f", ((clientbwstats*)curpeer->bandwidthStats.data)->incomingtotal);
            break;

        case 'j':
            str.fmt("%.2f", ((clientbwstats*)curpeer->bandwidthStats.data)->outgoingtotal);
            break;

        default:
            return false;
        }

        return true;
    }

    #undef ERROR
    #undef OK

    enum pstatus
    {
        OK,
        ERROR,
        ERROR_END_OF_BLOCK,
        ERROR_END_OF_FMT_STRING,
    };

    static pstatus parsestatsfmt(const char *&fmt, const char *fmtstart, fpsent *d,
                                 mod::strtool &str, mod::strtool &error, const char *currentmode,
                                 bool notmatching, const int tagopen, const int tagclose)
    {
        string buf;
        mod::strtool mode(buf, sizeof(buf));

        fmt++; // tagopen
        mode.add(tagopen);

        if (*fmt == tagopen)
           goto expectedgamemode;

        while (*fmt)
        {
            if (*fmt == tagclose) break;
            mode.add(*fmt);
            fmt++;
        }

        if (!*fmt)
        {
            error.fmt("expected '%c' after '%s'", tagclose, mode.getbuf());
            return pstatus::ERROR;
        }

        fmt++; // tagclose
        mode.add(tagclose);

        if (mode.length() == 2) // tagopen+tagclose
        {
            expectedgamemode:;
            error.fmt("expected game mode name after '%c'", tagopen);
            return pstatus::ERROR;
        }

        if (currentmode && !strcmp(mode.getbuf(), currentmode))
            return pstatus::ERROR_END_OF_BLOCK;

        bool cond = true;
        int op = '*';
        int skip = 1; // tagopen
        int condindex = 1;

        const char *p = mode.getbuf();

        while (p[condindex] == '!')
        {
            cond = !cond;
            skip++;
            condindex++;
        }

        switch (p[condindex])
        {
        case '=':
        case '*':
            op = p[condindex];
            skip++;
        }

        if (mode.length() == (size_t)skip+(condindex > 1))
        {
            mode.pop(); // tagclose
            error << "expected game mode name after '" << mode << "'";
            return ERROR;
        }

        bool match = false;

        if (!notmatching)
        {
            int index = clamp(gamemode-STARTGAMEMODE, 0, NUMGAMEMODES-1);
            const char *activemodename = gamemodes[index].name;

            bool cmpres;

            const char *p = mode.getbuf()+skip;
            size_t plength = mode.length()-skip-1; // -1=tagclose

            #define cmdcmp(cmd) !strncmp(p, cmd, plength)

            switch (*p++)
            {
                case 'c':
                {
                    if (cmdcmp("connected"))
                    {
                        cmpres = !!curpeer;
                        goto checkmatch;
                    }
                    break;
                }

                case 'd':
                {
                    if (cmdcmp("demo"))
                    {
                        cmpres = game::demoplayback;
                        goto checkmatch;
                    }
                    break;
                }

                case 'p':
                {
                    if (cmdcmp("playergiven"))
                    {
                        cmpres = !!d;
                        goto checkmatch;
                    }
                    else if (cmdcmp("playerisme"))
                    {
                        cmpres = (d == player1);
                        goto checkmatch;
                    }
                    break;
                }
            }

            #undef cmdcmp
            #define cmdcmp(cmd) !strncmp(p, cmd, plength)

            p--; // no case matched above, so decrease pointer again

            switch (op)
            {
            case '*':
                mode.pop(); // pop tagclose temporary for comparing
                mode.finalize();
                cmpres = !!strstr(activemodename, p);
                mode.add(tagclose);
                break;
            case '=':
                cmpres = cmdcmp(activemodename);
                break;
            default:
                abort(); // silence gcc warning
            }

            #undef cmdcmp

            checkmatch:;
            match = (cmpres == cond);
        }

        const char *tmp = formatstats(fmt, fmtstart, str, d, error, mode.getbuf(), !match);

        if (tmp == fmt)
        {
            error = "unexpected end of format string";
            return pstatus::ERROR_END_OF_FMT_STRING;
        }

        fmt = tmp;
        if (!fmt) return pstatus::ERROR;
        else if (!*fmt) return pstatus::ERROR_END_OF_FMT_STRING;
        fmt--;

        return pstatus::OK;
    }

    static const char *formatstats(const char *fmt, const char *fmtstart, mod::strtool &str,
                                   fpsent *d, mod::strtool &error, const char *currentmode,
                                   bool notmatching)
    {
        static const int RECURSIONLIMIT = 50;
        CHECKCALLDEPTH(RECURSIONLIMIT, error = "too many recursive game mode checks"; return NULL);

        extinfo::playerv2 *ep = d ? d->extinfo : NULL;

        const char *start = fmt;
        --fmt;

        while ((fmt > start ? *fmt : true) && *++fmt)
        {
            if (*fmt != '%')
            {
                if (*fmt == '\\')
                {
                    fmt++;

                    if (!*fmt)
                    {
                        error = "expected character after '\\'";
                        return NULL;
                    }

                    goto append;
                }

                static const int tagopen  = '[';
                static const int tagclose = ']';

                if (*fmt == tagopen)
                {
                    pstatus status = parsestatsfmt(fmt, fmtstart, d, str, error,
                                                   currentmode, notmatching, tagopen, tagclose);

                    switch (status)
                    {
                        case pstatus::OK: continue;
                        case pstatus::ERROR: return NULL;
                        case pstatus::ERROR_END_OF_FMT_STRING: break;
                        case pstatus::ERROR_END_OF_BLOCK: return fmt;
                    }
                }

                append:;
                if (!str.capacity())
                {
                    error = "result too long";
                    return NULL;
                }
                if (!notmatching) str.add(*fmt);
                continue;
            }

            int type = *++fmt;

            if (notmatching)
            {
                if (!type)
                {
                    error = "unexpected end of format string";
                    return NULL;
                }
                continue;
            }

            if (d && formatplayerstats(type, d, ep, str)) continue;
            if (formatnonplayerstats(type, str)) continue;
            if (curpeer && formatbandwidthstats(type, str)) continue;

            if (!type)
            {
                error = "expected character after '%'";
                return NULL;
            }
            else if (*fmt != '%')
            {
                error << "'%%" << *fmt << "' is an invalid type";
                if (!d) error += " (or requires cn)";
                if (!curpeer) error += " (or requires to be connected)";
                return NULL;
            }
            goto append;
        }

        if (currentmode)
        {
            error << "expected '" << currentmode << "'";
            return NULL;
        }

        return fmt;
    }

    static void parsegamestate(fpsent *d, const char *fmt, mod::strtool &str)
    {
        string errbuf;
        mod::strtool error(errbuf, sizeof(errbuf));

        if (!formatstats(fmt, fmt, str, d, error))
        {
            str.clear();
            str << "parse error: " << error;
        }
    }

    const char *getgamestate(void *d, const char *fmt, char *buf, int len)
    {
        mod::strtool str(buf, len);
        parsegamestate((fpsent*)d, fmt, str);
        return str.getbuf();
    }

    static void getgamestatefmt(int *cn, const char *fmt)
    {
        fpsent *d = *cn > -1 ? getclient(*cn) : NULL;

        char buf[1024];
        mod::strtool str(buf, sizeof(buf));

        if (d || *cn <= -1) parsegamestate(d, fmt, str);
        else str = "invalid cn given (use -1 instead)";
        result(str.getbuf());
    }
    COMMAND(getgamestatefmt, "is");

    int guiprivcolor(int priv)
    {
        switch(priv)
        {
            case PRIV_MASTER: return COLOR_MASTER;
            case PRIV_AUTH:   return COLOR_AUTH;
            case PRIV_ADMIN:  return COLOR_ADMIN;
        }
        return COLOR_NORMAL;
    }

    int getgamemodenum(const char *gamemode)
    {
        string buf1, buf2;
        strtool gamemodelc(buf1, sizeof(buf1));
        strtool gamemodeplain(buf2, sizeof(buf2));

        static const char ignorechars[] = " _-.";

        for(const char *p = gamemode; *p; p++)
        {
            loopi(STRLEN(ignorechars))
            {
                if (ignorechars[i] == *p)
                    goto next;
            }

            gamemodelc += *p;
            next:;
        }

        gamemodelc.lowerstring();

        int gm = -1;

        if (!isnumeric(gamemode))
        {
            if (*gamemode)
            {
                for(int i = STARTGAMEMODE*-1; i < NUMGAMEMODES; i++)
                {
                    gamemodeplain.clear();

                    for(const char *p = gamemodes[i].name; *p; p++)
                    {
                        loopi(STRLEN(ignorechars))
                        {
                            if (ignorechars[i] == *p)
                                goto nextc;
                        }

                        gamemodeplain += *p;
                        nextc:;
                    }

                    if (gamemodeplain == gamemodelc)
                    {
                        gm = i+STARTGAMEMODE;
                        break;
                    }
                }

                if (gm == -1)
                {
                    return STARTGAMEMODE-1;
                }
            }

        }
        else
        {
            int tmp = atoi(gamemode);
            gm = clamp(tmp, 0, NUMGAMEMODES-(STARTGAMEMODE*-1));

            if (gm != tmp)
                return STARTGAMEMODE-1;
        }

        return gm;
    }

    bool validprotocolversion(int num)
    {
        return (num == PROTOCOL_VERSION);
    }

    bool haveextinfoplayerips(int type, void *pplayers)
    {
        switch (type)
        {
            case EXTINFO_IPS_SCOREBOARD:
            {
                bool rv = false;
                clients.add(player1);
                loopv(clients)
                {
                    fpsent *c = clients[i];
                    if (!c || c->aitype != AI_NONE) continue;
                    auto *ep = c->extinfo;
                    if (ep && ep->ip.ui32 != uint32_t(-1))
                    {
                        rv = true;
                        break;
                    }
                }
                clients.pop();
                return rv;
            }

            case EXTINFO_IPS_SERVERBROWSER:
            {
                vector<extinfo::playerinfo> &players = *(vector<extinfo::playerinfo>*)pplayers;
                loopv(players)
                {
                    extinfo::playerinfo &pi = players[i];
                    if (pi.ep.cn < 128 && pi.ep.ip.ui32 != uint32_t(-1)) return true;
                }
                break;
            }
        }

        return false;
    }
}
