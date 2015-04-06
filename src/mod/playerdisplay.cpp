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

extern int fullconsole;
bool disableplayerdisplay = false;

namespace game
{
    extern int showextinfo;
}

using namespace game;
using namespace mod;

namespace gamemod
{
    static const char *COLOR_ALIVE = "\f0";
    static const char *COLOR_DEAD = "\f4";
    static const char *COLOR_ME = "\f7";
    static const char *COLOR_SPEC = "\f7";
    static const char *COLOR_UNKNOWN = "\f7";

    static const char *specteam = "spectators";
    static const char *noteams = "no teams";
    static const int maxteams = 10;

    static const int stdshowspectators = 1;
    static const int stdfontscale = 25;
    static const int stdrightoffset = 0;
    static const int stdheightoffset = 250;
    static const int stdwidthoffset = 0;
    static const int stdlineoffset = 0;
    static const int stdplayerlimit = 32;
    static const int stdmaxteams = 6;
    static const int stdmaxnamelen = 13;
    static const int stdalpha = 150;

    static float fontscale = stdfontscale/100.0f;

    MODVARP(showplayerdisplay, 0, 0, 1);
    MODVARP(playerdisplayshowspectators, 0, stdshowspectators, 1);
    MODVARFP(playerdisplayfontscale, 25, stdfontscale, 30, fontscale = playerdisplayfontscale/100.0f);
    MODVARP(playerdisplayrightoffset, -100, stdrightoffset, 100);
    MODVARP(playerdisplayheightoffset, 0, stdheightoffset, 600);
    MODVARP(playerdisplaywidthoffset, -50, stdwidthoffset, 300);
    MODVARP(playerdisplaylineoffset, -30, stdlineoffset, 50);
    MODVARP(playerdisplayplayerlimit, 1, stdplayerlimit, 64);
    MODVARP(playerdisplaymaxteams, 1, stdmaxteams, maxteams);
    MODVARP(playerdisplaymaxnamelen, 4, stdmaxnamelen, MAXNAMELEN);
    MODVARP(playerdisplayalpha, 20, stdalpha, 0xFF);

    static void playerdisplayreset()
    {
        playerdisplayshowspectators = stdshowspectators;
        playerdisplayfontscale = stdfontscale;
        playerdisplayrightoffset = stdrightoffset;
        playerdisplayheightoffset = stdheightoffset;
        playerdisplaywidthoffset = stdwidthoffset;
        playerdisplaylineoffset = stdlineoffset;
        playerdisplayplayerlimit = stdplayerlimit;
        playerdisplaymaxteams = stdmaxteams;
        playerdisplaymaxnamelen = stdmaxnamelen;
        playerdisplayalpha = stdalpha;
    }
    COMMAND(playerdisplayreset, "");

    struct playerteam
    {
        bool needfrags;
        int frags;
        int score;
        const char *name;
        vector<fpsent*> players;
    };

    static int compareplayer(const fpsent *a, const fpsent *b)
    {
        if (a->frags > b->frags) return 1;
        else if (a->frags < b->frags) return 0;
        else return !strcmp(a->name, b->name) ? 1 : 0;
    }

    static int numteams = 0;
    static playerteam teams[maxteams];
    static vector<teamscore> teamscores;

    static void sortteams(playerteam **result, bool hidefrags)
    {
        loopi(numteams) result[i] = &teams[i];

        if (numteams <= 1)
            return;

        loopi(numteams)
        {
            loopj(numteams-i-1)
            {
                playerteam *a = result[j];
                playerteam *b = result[j+1];
                if ((hidefrags ? a->score < b->score : a->frags < b->frags) || a->name == specteam)
                {
                    result[j] = b;
                    result[j+1] = a;
                }
            }
        }
    }

    static playerteam* accessteam(const char *teamname, bool hidefrags, bool add = false)
    {
        if (!add) loopi(numteams) if (!strcmp(teams[i].name, teamname)) return &teams[i];
        if (numteams < maxteams)
        {
            playerteam &team = teams[numteams];
            team.name = teamname;
            teaminfo *ti = getteaminfo(teamname);
            if (ti) team.frags = ti->frags;
            else team.frags = 0;
            team.score = 0;
            if (hidefrags)
            {
                team.needfrags = !team.frags;
                loopv(teamscores) if (!strcmp(teamscores[i].team, teamname))
                {
                    team.score = teamscores[i].score;
                    break;
                }
            }
            else team.needfrags = !ti;
            team.players.setsize(0);
            numteams++;
            return &team;
        }
        return NULL;
    }

    void renderplayerdisplay(int conw, int conh, int FONTH, int w, int h)
    {
        if ((disableplayerdisplay || !showplayerdisplay) ||
            (m_edit || isscoreboardshown() || !isconnected(false, true)))
             return;

        if (fullconsole || w < 1024 || h < 640)
            return;

        static const int lw[] = { -700, -1000 };

        glPushMatrix();
        glScalef(fontscale, fontscale, 1);

        if (!demoplayback)
            clients.add(player1);

        numteams = 0;
        teamscores.setsize(0);

        bool hidefrags = m_teammode && cmode && cmode->hidefrags();

        if (hidefrags)
            cmode->getteamscores(teamscores);

        loopvrev(clients)
        {
            fpsent *d = clients[i];

            if (!d || (d->state == CS_SPECTATOR && !playerdisplayshowspectators))
                continue;

            playerteam *team;
            const char *teamname = d->state == CS_SPECTATOR ? specteam : m_teammode ? d->team : "";

            team = accessteam(teamname, hidefrags);

            if (!team)
            {
                if (!m_teammode && numteams)
                {
                    if (teamname == specteam)
                    {
                        loopi(numteams) if (teams[i].name == specteam)
                        {
                            team = &teams[i];
                            break;
                        }
                    }
                    else
                    {
                        loopi(numteams) if (teams[i].name != specteam)
                        {
                            team = &teams[i];
                            break;
                        }
                    }
                }

                if (!team)
                {
                    team = accessteam(teamname, hidefrags, true);

                    if (!team)
                        break;
                }
            }

            team->players.add(d);
            if (team->needfrags) team->frags += d->frags;
        }

        if (!demoplayback)
            clients.pop(); // pop player1

        loopi(numteams)
            teams[i].players.sort(compareplayer);

        static playerteam *teams[maxteams];
        sortteams(teams, hidefrags);

        int j = 0, playersshown = 0;
        numteams = min(playerdisplaymaxteams, numteams);

        loopi(numteams)
        {
            playerteam *team = teams[i];

            if (i > 0) j += 2;

            int left;
            int top = conh;
            top += rescaleheight(-100+LINEOFFSET(j)+
                                 playerdisplaylineoffset*j+playerdisplayheightoffset+
                                 (m_ctf ? -50 : 0), h);

            const char *teamname = team->name;

            if (!m_teammode && teamname != specteam)
                teamname = noteams;

            static draw dt;

            dt.clear();
            dt.setalpha(playerdisplayalpha);

            left = conw+rescalewidth(lw[1]+playerdisplaywidthoffset, w);
            dt << teamname;
            dt.drawtext(left, top);

            if (team->name != specteam)
            {
                dt.cleartext();
                left = conw+rescalewidth(lw[0]+playerdisplaywidthoffset+playerdisplayrightoffset, w);

                if (hidefrags)
                    dt << "(" << team->frags << " / " << team->score << ")";
                else
                    dt << "(" << team->frags << ")";

                dt.drawtext(left, top);
            }

            int teamlimit = playerdisplayplayerlimit/numteams;

            loopv(team->players)
            {
                fpsent *d = team->players[i];

                if (!d)
                    continue;

                j++;
                playersshown++;

                const char *color;

                if (d != player1)
                {
                    switch (d->state)
                    {
                        case CS_ALIVE: color = COLOR_ALIVE; break;
                        case CS_DEAD: color = COLOR_DEAD; break;
                        case CS_SPECTATOR: color = COLOR_SPEC; break;
                        default: color = COLOR_UNKNOWN;
                    }
                }
                else color = COLOR_ME;

                int c = d->name[playerdisplaymaxnamelen];
                d->name[playerdisplaymaxnamelen] = 0;

                auto *ep = d->extinfo;

                int top = conh;

                top += rescaleheight(-100+LINEOFFSET(j)+
                                     playerdisplaylineoffset*j+
                                     playerdisplayheightoffset+(m_ctf ? -50 : 0), h);

                dt.cleartext();
                left = conw+rescalewidth(lw[1]+playerdisplaywidthoffset, w);

                dt << color << d->name;
                dt.drawtext(left, top);

                dt.cleartext();
                left = conw+rescalewidth(lw[0]+playerdisplaywidthoffset+playerdisplayrightoffset, w);

                if (showextinfo)
                    dt << color << "(" << d->frags << " / " << (ep ? ep->acc : 0) << "%)";
                else
                    dt << color << "(" << d->frags << " / " << "??%";

                dt.drawtext(left, top);

                d->name[playerdisplaymaxnamelen] = c;

                if (playersshown >= playerdisplayplayerlimit)
                    goto retn;

                if (!--teamlimit)
                    break;
            }
        }

        retn:;
        glPopMatrix();
    }
}
