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

#include <game.h>

namespace game
{
    extern int scaletimeleft;
    extern int ipignorecolor;
    extern int showcountry;
    extern int showip;
    extern int showtks;
    extern int showdamagedealt;
}

using namespace game;
using namespace gamemod;

namespace mod {
namespace extinfo {

#define loopexintofplayers(o, b) \
    loopv(ti.players) \
    { \
        auto &o = ti.players[i]->ep; \
        b; \
    }

#define loopexintofplayerinfos(o, b) \
    loopv(ti.players) \
    { \
        playerinfo &o = *ti.players[i]; \
        b; \
    }

struct teaminfo
{
    const char *team;
    vector<playerinfo*> players;
    int score;
    teaminfo() : team(""), score(0) { }
};

static int compareteam(const teaminfo a, const teaminfo b)
{
    if (a.score > b.score) return 1;
    else if (a.score < b.score) return 0;
    else return !strcmp(a.team, b.team) ? 1 : 0;
}

static int compareplayer(const playerinfo *a, const playerinfo *b)
{
    if (a->ep.frags > b->ep.frags) return 1;
    else if (a->ep.frags < b->ep.frags) return 0;
    else return !strcmp(a->ep.name, b->ep.name) ? 1 : 0;
}

void renderplayerpreview(g3d_gui& g, strtool& command, const ENetAddress& eaddr, bool waiting,
                         const char *pass, int ping, const char *addr, int port, const char *desc,
                         int numattr, const int *attr, int numteams, const void *ti,
                         const char *mapname, int numplayers, void *pplayers)
{
    string errbuf;
    strtool errstr(errbuf, sizeof(errbuf));
    int errcolor = 0xFF0000;
    const char *connectcmd;

    if (!waiting && (numattr != 5 && numattr != 7))
        errstr.fmt("invalid numattr (%d)", numattr);

    extinfo::extteaminfo *extteaminfo = (extinfo::extteaminfo*)ti;
    vector<playerinfo>& players = *(vector<playerinfo>*)pplayers;

    int protocol = 0, gamemode = 0, secleft = 0, maxplayers = 0, mastermode = 0;
    bool gamepaused = false;
    int gamespeed = 100;

    if (!waiting && errstr.empty())
    {
        protocol = attr[0];
        gamemode = attr[1];
        secleft = attr[2];
        maxplayers = attr[3];
        mastermode = attr[4];

        if (numattr>=6) gamepaused = attr[5]!=0;
        if (numattr>=7) gamespeed = attr[6];
    }

    secleft *= 100/(scaletimeleft && gamespeed>0 ? gamespeed : 100);

    int secs = clamp(secleft, 0, 59*60+59);
    int mins = secs/60; secs %= 60;

    static const int color = 0xFFFF80;

    bool haspass;

    vector<teaminfo> teams;
    vector<playerinfo*> specs;

    if (errstr.empty() && (protocol != PROTOCOL_VERSION || waiting))
    {
        if (!waiting)
        {
            errstr.fmt("protocol version mismatch (%d!=%d)", protocol, PROTOCOL_VERSION);
        }
        else
        {
            errstr = "waiting for reply from server...";
            errcolor = color;
        }
    }

    if (!errstr.empty())
    {
        g.pushlist();
        g.spring();
        g.text(errstr.getbuf(), errcolor);
        g.spring();
        g.poplist();
        g.separator();
        g.pushlist();
        g.spring();
        goto serverbrowser;
        errormsg:;
        g.spring();
        g.poplist();
        return;
    }

    g.pushlist();
    g.spring();
    string hostname;
    if (enet_address_get_host_ip(&eaddr, hostname, sizeof(hostname)) >= 0)
    {
        if (*desc) g.textf("%.25s", color, NULL, desc);
        else g.textf("%s:%d", color, NULL, hostname, port);
    }
    g.separator();
    g.textf("%s", color, NULL, server::modename(gamemode));
    g.separator();
    g.textf("%s", color, NULL, *mapname ? mapname : "unknown");
    if (gamepaused)
    {
        g.separator();
        g.text("paused", color, NULL);
    }
    if (gamespeed != 100)
    {
        g.separator();
        g.textf("%.2fx", color, NULL, gamespeed/100.0f);
    }
    if (mastermode != MM_OPEN)
    {
        g.separator();
        g.textf("%s%s", color, NULL, mastermodecolor(mastermode, ""), server::mastermodename(mastermode));
    }
    g.separator();
    if (secleft > 0) g.textf("%d:%02d", color, NULL, mins, secs);
    else g.text("intermission", color, NULL);
    g.separator();
    g.textf("%d ms", color, NULL, ping);
    g.separator();
    g.textf("%d/%d", color, NULL, numplayers, maxplayers);
    g.spring();
    g.poplist();

    g.separator();

    if (m_teammode)
    {
        loopv(players)
        {
            bool f = false;
            loopvj(teams) if (!strcmp(teams[j].team, players[i].ep.team))
            {
                f = true;
                break;
            }
            if (!f)
            {
                teaminfo &ti = teams.add();
                ti.team = players[i].ep.team;
            }
        }

        loopv(teams)
        {
            teaminfo& t = teams[i];
            loopvj(players) if (!strcmp(t.team, players[j].ep.team))
            {
                if (players[j].ep.state != CS_SPECTATOR) t.players.add(&players[j]);
                else specs.add(&players[j]);
            }
            loopj(numteams) if (!strcmp(t.team, extteaminfo[j].team))
            {
                t.score = extteaminfo[j].score;
                break;
            }
        }
    }
    else if (!players.empty())
    {
        teaminfo &ti = teams.add();
        loopv(players) if (players[i].ep.state != CS_SPECTATOR) ti.players.add(&players[i]);
        else specs.add(&players[i]);
    }

    if (!specs.empty())
    {
        teaminfo &ti = teams.add();
        ti.team = "spectators";
        ti.players = specs;
        specs.setsize(0);
    }

    if (teams.empty() && !numplayers)
    {
        g.pushlist();
        g.spring();
        g.text("no players connected", color);
        g.spring();
        g.poplist();
    }
    else
    {
        teams.sort(compareteam);
        loopv(teams) teams[i].players.sort(compareplayer);
    }

    loopv(teams)
    {
        teaminfo& ti = teams[i];
        if (ti.players.empty()) continue;

        bool extended = false;
        loopexintofplayers(ep, { if (ep.ext.ishopmodcompatible() || ep.ext.isoomod()) { extended = true; break; } });

        if (i > 0) g.separator();

        bool isspecteam;
        if ((isspecteam = !strcmp(ti.team, "spectators")) || m_teammode)
        {
            g.pushlist();
            g.background(!strcmp(ti.team, "good") ? 0x3030C0 : (!strcmp(ti.team, "spectators") ? 0xCC9966 : 0xC03030));
            g.spring();
            if (isspecteam) g.text(ti.team, color, NULL);
            else g.textf("%s (%d)", color, NULL, ti.team, ti.score);
            g.spring();
            g.poplist();
        }

        g.pushlist(); // begin player listing

        g.spring(); // center players
        g.pushlist(); // vertical
        g.pushlist(); // horizontal
        g.spring();

        g.pushlist();
        g.strut(14);
        g.text("name", color);

        loopexintofplayers(ep,
        {
            bool ignored = ipignore::isignored(ep.ip.ui32);
            if(ignored)
            {
                g.pushlist();
                g.background(ipignorecolor, 3);
            }
            g.text(ep.getname(), guiprivcolor(ep.priv), NULL);
            if(ignored) g.poplist();
        });
        g.poplist();

        if (showcountry && haveextinfoplayerips(EXTINFO_IPS_SERVERBROWSER, &players))
        {
            g.pushlist();
            g.text("country", color);
            g.strut(8);
            loopexintofplayerinfos(epi, rendercountry(g, epi.countrycode, epi.country ? epi.country : "??", showcountry));
            g.poplist();
        }

        if (showip)
        {
            g.pushlist();
            g.strut(11);
            g.text("ip", color);
            loopexintofplayers(ep, g.textf("%u.%u.%u", 0xFFFFDD, NULL, ep.ip.ia[0], ep.ip.ia[1], ep.ip.ia[2]));
            g.poplist();
        }

        g.pushlist();
        g.strut(6);
        g.text("ping", color);
        loopexintofplayers(ep, g.textf("%d", 0xFFFFDD, NULL, ep.ping));
        g.poplist();

        g.pushlist();
        g.strut(6);
        g.text("frags", color);
        loopexintofplayers(ep, g.textf("%d", 0xFFFFDD, NULL, ep.frags));
        g.poplist();

        g.pushlist();
        g.strut(8);
        g.text("deaths", color);
        loopexintofplayers(ep, g.textf("%d", 0xFFFFDD, NULL, ep.deaths));
        g.poplist();

        if (extended && showdamagedealt)
        {
            g.pushlist();
            g.strut(6);
            g.text("dd", color);
            loopexintofplayers(ep,
            {
                bool get = ep.ext.damage>=1000 || showdamagedealt==2;
                g.textf(get ? "%.2fk" : "%.0f", 0xFFFFDD, NULL, get ? ep.ext.damage/1000.f : ep.ext.damage*1.f);
            });
            g.poplist();
        }

        g.pushlist();
        g.strut(6);
        g.text("kpd", color);
        loopexintofplayers(ep, g.textf("%.2f", 0xFFFFDD, NULL, ep.frags/float(ep.deaths?ep.deaths:1)));
        g.poplist();

        g.pushlist();
        g.strut(6);
        g.text("acc", color);
        loopexintofplayers(ep, g.textf("%d%%", 0xFFFFDD, NULL, ep.acc));
        g.poplist();

        if (m_teammode)
        {
            if (showtks)
            {
                g.pushlist();
                g.strut(9);
                g.text("teamkills", color);
                loopexintofplayers(ep, g.textf("%d", 0xFFFFDD, NULL, ep.teamkills));
                g.poplist();
            }

            if (m_ctf || m_collect)
            {
                g.pushlist();
                g.strut(6);
                g.text(m_collect ? "skulls" : "flags", color);
                loopexintofplayers(ep, g.textf("%d", 0xFFFFDD, NULL, ep.flags));
                g.poplist();
            }
        }

        g.pushlist();
        g.text("cn", color);
        loopexintofplayers(ep, g.textf("%d", 0xFFFFDD, NULL, ep.cn));
        g.poplist();

        g.spring(); // center players

        g.poplist();
        g.poplist();

        g.spring();

        g.poplist(); // end playerlisting
    }

    g.separator();

    g.pushlist();
    g.spring();

    haspass = pass && *pass;
    connectcmd = "connect";

    if (*bouncerhost)
    {
        if (haspass)
        {
            if (g.button("connect (bouncer + pw)", 0xFFFFDD)&G3D_UP)
            {
                connectcmd = "bouncerconnect";
                goto connect;
            }
            g.separator();
        }
        if (g.button(haspass ? "connect (bouncer - no pw)" :  "connect (bouncer)", 0xFFFFDD)&G3D_UP)
        {
            connectcmd = "bouncerconnect";
            goto nopwconnect;
        }
        if (haspass)
        {
            g.spring();
            g.poplist();
            g.pushlist();
            g.spring();
        }
    }

    if (haspass)
    {
        if (g.button("connect (with password)", 0xFFFFDD)&G3D_UP)
        {
            connect:;
            strtool addrbuf(32), passbuf(haspass ? 32 : 0);
            escapecubescriptstring(addr, addrbuf);
            if (haspass) escapecubescriptstring(pass, passbuf);
            command.fmt("%s %s %d", connectcmd, addrbuf.str(), port);
            if (haspass)
            {
                command += ' ';
                command += passbuf.getbuf();
            }
        }
    }

    if (numplayers < maxplayers && mastermode != MM_PRIVATE && mastermode != MM_PASSWORD)
    {
        if (haspass || *bouncerhost) g.separator();
        if (g.button(haspass ? "connect (without password)" : "connect", 0xFFFFDD)&G3D_UP)
        {
            nopwconnect:;
            pass = "";
            goto connect;
        }
    }
    else if (!haspass || (mastermode != MM_PRIVATE && mastermode != MM_PASSWORD))
    {
        int color;
        const char *text;
        if (numplayers < maxplayers)
        {
            switch (mastermode)
            {
                case MM_PRIVATE: color = 0xFF0000; text = "server is in private mode"; break;
                case MM_PASSWORD: color = 0xFF0000; text = "server is password protected"; break;
                default: color = 0x7AFFFF; text = "unknown";
            }
        }
        else
        {
            color = 0xFF0000;
            text = "server FULL";
        }
        if (haspass || *bouncerhost) g.separator();
        if (g.text(text, color)&G3D_UP) goto nopwconnect;
    }

    g.separator();

    serverbrowser:;
    if (g.button("server browser", 0xFFFFDD)&G3D_UP) command = "showgui servers";
    if (!errstr.empty()) goto errormsg;

    g.spring();
    g.poplist();
}

} // namespace extinfo
} // namespace mod
