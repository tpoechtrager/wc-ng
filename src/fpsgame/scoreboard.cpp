// creation of scoreboard
#include "game.h"
using namespace mod::extinfo; //NEW

namespace game
{
    VARP(scoreboard2d, 0, 1, 1);
    VARP(showservinfo, 0, 1, 1);
    VARP(showclientnum, 0, 0, 2); //NEW 1 -> 2
    VARP(showpj, 0, 0, 1);
    VARP(showping, 0, 1, 2);
    VARP(showspectators, 0, 1, 1);
    VARP(showspectatorping, 0, 0, 1);
    VARP(highlightscore, 0, 1, 1);
    VARP(showconnecting, 0, 0, 1);
    VARP(hidefrags, 0, 1, 1);
    //VARP(showdeaths, 0, 0, 1); //NEW commented
    VARP(tiesort, 0, 0, 2);
    //NEW
    MODHVARP(scoreboardtextcolorhead, 0, 0xFFFF80, 0xFFFFFF);
    MODHVARP(scoreboardtextcolor, 0, 0xFFFFDD, 0xFFFFFF);
    MODVARP(showextinfo, 0, 1, 1);
    MODVARP(showfrags, 0, 1, 1);
    MODVARP(showdeaths, 0, 1, 1);
    MODVARP(showdamagedealt, 0, 0, 2);
    MODVARP(showkpd, 0, 0, 1);
    MODVARP(showacc, 0, 1, 1);
    MODVARP(showtks, 0, 0, 1);
    MODVARP(showcountry, 0, 3, 5);
    MODVARP(showserveruptime, 0, 0, 1);
    MODVARP(showservermod, 0, 0, 1);
    MODVARP(oldscoreboard, 0, 0, 1);
    MODVARP(showdemotime, 0, 1, 1);
#ifdef ENABLE_IPS
    MODVARP(showip, 0, 0, 1);
    MODVARP(showspectatorip, 0, 0, 1);
#else
    int showip = 0;
    int showspectatorip = 0;
#endif //ENABLE_IPS
    //NEW END

    static hashset<teaminfo> teaminfos;

    void clearteaminfo()
    {
        teaminfos.clear();
    }

    void setteaminfo(const char *team, int frags)
    {
        teaminfo *t = teaminfos.access(team);
        if(!t) { t = &teaminfos[team]; copystring(t->team, team, sizeof(t->team)); }
        t->frags = frags;
    }

    teaminfo *getteaminfo(const char *team) { return teaminfos.access(team); } //NEW

    static inline bool playersort(const fpsent *a, const fpsent *b)
    {
        if(a->state==CS_SPECTATOR)
        {
            if(b->state==CS_SPECTATOR) return strcmp(a->name, b->name) < 0;
            else return false;
        }
        else if(b->state==CS_SPECTATOR) return true;
        if(m_ctf || m_collect)
        {
            if(a->flags > b->flags) return true;
            if(a->flags < b->flags) return false;
            if(tiesort)
            {
                if(a == player1) return tiesort == 1;
                if(b == player1) return tiesort != 1;
            }
        }
        if(a->frags > b->frags) return true;
        if(a->frags < b->frags) return false;
        if(tiesort)
        {
            if(a == player1) return tiesort == 1;
            if(b == player1) return tiesort != 1;
        }
        return strcmp(a->name, b->name) < 0;
    }

    void getbestplayers(vector<fpsent *> &best)
    {
        loopv(players)
        {
            fpsent *o = players[i];
            if(o->state!=CS_SPECTATOR) best.add(o);
        }
        best.sort(playersort);
        while(best.length() > 1 && best.last()->frags < best[0]->frags) best.drop();
    }

    void getbestteams(vector<const char *> &best)
    {
        if(cmode && cmode->hidefrags())
        {
            vector<teamscore> teamscores;
            cmode->getteamscores(teamscores);
            teamscores.sort(teamscore::compare);
            while(teamscores.length() > 1 && teamscores.last().score < teamscores[0].score) teamscores.drop();
            loopv(teamscores) best.add(teamscores[i].team);
        }
        else
        {
            int bestfrags = INT_MIN;
            enumerate(teaminfos, teaminfo, t, bestfrags = max(bestfrags, t.frags));
            if(bestfrags <= 0) loopv(players)
            {
                fpsent *o = players[i];
                if(o->state!=CS_SPECTATOR && !teaminfos.access(o->team) && best.htfind(o->team) < 0) { bestfrags = 0; best.add(o->team); }
            }
            enumerate(teaminfos, teaminfo, t, if(t.frags >= bestfrags) best.add(t.team));
        }
    }

    struct scoregroup : teamscore
    {
        vector<fpsent *> players;
    };
    static vector<scoregroup *> groups;
    static vector<fpsent *> spectators;

    static inline bool scoregroupcmp(const scoregroup *x, const scoregroup *y)
    {
        if(!x->team)
        {
            if(y->team) return false;
        }
        else if(!y->team) return true;
        if(x->score > y->score) return true;
        if(x->score < y->score) return false;
        if(x->players.length() > y->players.length()) return true;
        if(x->players.length() < y->players.length()) return false;
        return x->team && y->team && strcmp(x->team, y->team) < 0;
    }

    static int groupplayers()
    {
        int numgroups = 0;
        spectators.setsize(0);
        loopv(players)
        {
            fpsent *o = players[i];
            if(!showconnecting && !o->name[0]) continue;
            if(o->state==CS_SPECTATOR) { spectators.add(o); continue; }
            const char *team = m_teammode && o->team[0] ? o->team : NULL;
            bool found = false;
            loopj(numgroups)
            {
                scoregroup &g = *groups[j];
                if(team!=g.team && (!team || !g.team || strcmp(team, g.team))) continue;
                g.players.add(o);
                found = true;
            }
            if(found) continue;
            if(numgroups>=groups.length()) groups.add(new scoregroup);
            scoregroup &g = *groups[numgroups++];
            g.team = team;
            if(!team) g.score = 0;
            else if(cmode && cmode->hidefrags()) g.score = cmode->getteamscore(o->team);
            else { teaminfo *ti = teaminfos.access(team); g.score = ti ? ti->frags : 0; }
            g.players.setsize(0);
            g.players.add(o);
        }
        loopi(numgroups) groups[i]->players.sort(playersort);
        spectators.sort(playersort);
        groups.sort(scoregroupcmp, 0, numgroups);
        return numgroups;
    }

    //NEW
    static inline const ENetAddress &getserveraddress()
    {
        if(curpeer) return curpeer->address;
        if(demohasservertitle) return demoserver;
        static const ENetAddress nulladdr{};
        return nulladdr;
    }

    template<typename T>
    static inline bool displayextinfo(T cond = T(1))
    {
        return cond && showextinfo && ((isconnected(false) && hasextinfo) || (demoplayback && demohasextinfo));
    }

    static const char *countryflag(const char *code, const char *name = NULL, bool staticstring = true)
    {
        static hashtable<const char* /* code */, const char* /* filename */> cache;
        bool nullcode;

        // claim we don't have continent flags
        if(name && name[0] && mod::geoip::iscontinent(name)) nullcode = true;
        else nullcode = !code || !code[0];

        if(nullcode)
        {
            code = "UNKNOWN";
            staticstring = true;
        }

        static string buf;
        static mod::strtool s(buf, sizeof(buf));

        const char *filename = cache.find(code, NULL);

        if(filename)
        {
            ret:;
            if(filename == (const char*)-1) return NULL;
            else return filename;
        }

        static const size_t fnoffset = STRLEN("packages/icons/");

        if(!s)
        {
            s.copy("packages/icons/", fnoffset);
            s.fixpathdiv();
        }
        else s -= s.length()-fnoffset;

        s.append(code, nullcode ? STRLEN("UNKNOWN") : 2);
        s.append(".png", 4);

        const char *file = s.str();

#if defined(__GNUC__) && !defined(__clang__) && !defined(__ICC)
        // workaround for a bogus gcc warning
        // gcc thinks "file" is an empty string constant, while it clearly isn't
        // warning: offset outside bounds of constant string
        __UNREACHABLE(!s);
#endif

        if(fileexists(findfile(file, "rb"), "rb")) filename = newstring(file+fnoffset);
        else filename = (const char*)-1;

        if(!staticstring) code = newstring(code);

        cache.access(code, filename);
        goto ret;
    }

    ICOMMAND(countryflagpath, "ss", (const char *c, const char *n), const char *fn = countryflag(c, n, false); result(fn ? fn : ""));

    void rendercountry(g3d_gui &g, const char *code, const char *name, int mode, bool *clicked, int scoreboard)
    {
        const char *dcode = code && *code ? code : "??";
        const char *dname = name && *name ? name : "??";

        int colour;

        switch(scoreboard)
        {
            case 1: colour = scoreboardtextcolor; break;
            case 2: colour = scoreboardtextcolorhead; break;
            default: colour = 0xFFFFDD;
        }

        /*
         * Mode:
         * 1: Country Code
         * 2: Country Name
         * 3: Country Flag | Country Code
         * 4: Country Flag | Country Name
         * 5: Country Flag
         */

        static string cnamebuf;
        static mod::strtool cname(cnamebuf, sizeof(cnamebuf));
        const char *cflag = mode>2 ? countryflag(code, name) : NULL;

        if(mode<5)
        {
            cname = (mode%2 ? dcode : dname);
            cname += ' ';
        }
        else cname.clear();

        if(clicked) *clicked = !!(g.button(cname.str(), colour, cflag)&G3D_UP);
        else g.text(cname.str(), colour, cflag);
    }

    static inline void rendercountry(g3d_gui &g, fpsent *o, int mode)
    {
        rendercountry(g, o->countrycode, o->country, mode, NULL, 1);
    }

    static inline void renderip(g3d_gui &g, fpsent *o)
    {
        if(o->extinfo) g.textf("%u.%u.%u", scoreboardtextcolor, NULL, o->extinfo->ip.ia[0], o->extinfo->ip.ia[1], o->extinfo->ip.ia[2]);
        else g.text("??", scoreboardtextcolor);
    }
    //NEW END

    int statuscolor(int privilege, int state, int color) //NEW
    {
        if(privilege)
        {
            color = privilege>=PRIV_ADMIN ? 0xFF8000 : (privilege>=PRIV_AUTH ? 0xC040C0 : 0x40FF80);
            if(state==CS_DEAD) color = (color>>1)&0x7F7F7F;
        }
        else if(state==CS_DEAD) color = 0x606060;
        return color;
    }

    int statuscolor(fpsent *d, int color)
    {
#if 0
        if(d->privilege)
        {
            color = d->privilege>=PRIV_ADMIN ? 0xFF8000 : (d->privilege>=PRIV_AUTH ? 0xC040C0 : 0x40FF80);
            if(d->state==CS_DEAD) color = (color>>1)&0x7F7F7F;
        }
        else if(d->state==CS_DEAD) color = 0x606060;
        return color;
#endif
        return statuscolor(d->privilege, d->state, color); //NEW
    }

    void renderscoreboard(g3d_gui &g, bool firstpass)
    {
        bool havecountrynames = displayextinfo(showcountry) && mod::extinfo::havecountrynames(mod::extinfo::EXTINFO_COUNTRY_NAMES_SCOREBOARD); //NEW
        const ENetAddress *address = connectedpeer();
        if(showservinfo && (address || demohasservertitle)) //NEW || demohasservertitle
        {
            //NEW
            if(demohasservertitle)
            {
                if(demoplayback) g.titlef("%.25s", scoreboardtextcolorhead, NULL, servinfo);
            }
            else
            //NEW END
            {
                string hostname;
                if(enet_address_get_host_ip(address, hostname, sizeof(hostname)) >= 0)
                {
                    if(servinfo[0]) g.titlef("%.25s", scoreboardtextcolorhead, NULL, servinfo);
                    else g.titlef("%s:%d", scoreboardtextcolorhead, NULL, hostname, address->port);
                }
            }
        }

        g.pushlist();
        g.spring();
        //NEW
        if(demoplayback && showdemotime && gametimestamp)
        {
            string buf;
            time_t ts = gametimestamp+((lastmillis-mapstart)/1000);
            struct tm *tm = localtime(&ts);
            strftime(buf, sizeof(buf), "%x %X", tm);
            g.text(buf, scoreboardtextcolorhead);
            g.separator();
        }
        //NEW END
        g.text(server::modename(gamemode), scoreboardtextcolorhead);
        g.separator();
        const char *mname = getclientmap();
        g.text(mname[0] ? mname : "[new map]", scoreboardtextcolorhead);
        extern int gamespeed;
        if(!demoplayback && mastermode != MM_OPEN) { g.separator(); g.textf("%s%s\fr", scoreboardtextcolorhead, NULL, mastermodecolor(mastermode, "\f0"), server::mastermodename(mastermode)); } //NEW
        if(gamespeed != 100) { g.separator(); g.textf("%d.%02dx", scoreboardtextcolorhead, NULL, gamespeed/100, gamespeed%100); }
        if(m_timed && mname[0] && (maplimit >= 0 || intermission))
        {
            g.separator();
            if(intermission) g.text("intermission", scoreboardtextcolorhead);
            else
            {
                extern int scaletimeleft; //NEW
                int secs = max(maplimit-lastmillis+999, 0)/1000*100/(scaletimeleft ? gamespeed : 100), mins = secs/60; //NEW 100/(scaletimeleft ? gamespeed : 100)
                secs %= 60;
                g.pushlist();
                g.strut(mins >= 10 ? 4.5f : 3.5f);
                g.textf("%d:%02d", scoreboardtextcolorhead, NULL, mins, secs);
                g.poplist();
            }
        }
        //NEW
        if(displayextinfo(showserveruptime))
        {
            int uptime = mod::extinfo::getserveruptime();
            if(uptime>0)
            {
                string buf;
                mod::strtool tmp(buf, sizeof(buf));
                tmp.fmtseconds(uptime);
                g.separator();
                g.pushlist();
                g.textf("server uptime: %s", scoreboardtextcolorhead, NULL, tmp.str());
                g.poplist();
            }
        }
        if(displayextinfo(showservermod))
        {
            const char *servermod = mod::extinfo::getservermodname();
            if(servermod)
            {
                g.separator();
                g.pushlist();
                g.textf("server mod: %s", scoreboardtextcolorhead, NULL, servermod);
                g.poplist();
            }
        }
        if(displayextinfo(showcountry))
        {
            uint32_t ip = 0;
            if(curpeer) ip = curpeer->address.host;
            else if(demoplayback && demohasservertitle) ip = demoserver.host;
            if(ip)
            {
                static const char *country;
                static const char *countrycode;
                static uint32_t lastip = 0;
                if(lastip != ip)
                {
                    mod::geoip::country(ip, &country, &countrycode);
                    lastip = ip;
                }
                if(country && countrycode)
                {
                    g.separator();
                    g.pushlist();
                    rendercountry(g, countrycode, country, showcountry, NULL, 2);
                    g.poplist();
                }
            }
        }
        //NEW END
        if(ispaused()) { g.separator(); g.text("paused", scoreboardtextcolorhead); }
        g.spring();
        g.poplist();

        g.separator();

        int numgroups = groupplayers();
        loopk(numgroups)
        {
            if((k%2)==0) g.pushlist(); // horizontal

            scoregroup &sg = *groups[k];
            int bgcolor = sg.team && m_teammode ? (isteam(player1->team, sg.team) ? 0x3030C0 : 0xC03030) : 0,
                fgcolor = scoreboardtextcolorhead;

            g.pushlist(); // vertical
            g.pushlist(); // horizontal

            #define loopscoregroup(o, b) \
                loopv(sg.players) \
                { \
                    fpsent *o = sg.players[i]; \
                    b; \
                }

            g.pushlist();
            if(sg.team && m_teammode)
            {
                g.pushlist();
                g.background(bgcolor, numgroups>1 ? 3 : 5);
                g.strut(1);
                g.poplist();
            }
            g.text("", 0, " ");
            loopscoregroup(o,
            {
                int bgcolor = gamemod::guiplayerbgcolor(o, getserveraddress()); //NEW
                if((o==player1 && highlightscore && (multiplayer(false) || demoplayback || players.length() > 1)) || bgcolor > -1) //NEW || bgcolor > -1
                {
                    g.pushlist();
                    g.background(bgcolor > -1 ? bgcolor : 0x808080, numgroups>1 ? 3 : 5); //NEW bgcolor > -1 ? bgcolor :
                }
                const playermodelinfo &mdl = getplayermodelinfo(o);
                const char *icon = sg.team && m_teammode ? (isteam(player1->team, sg.team) ? mdl.blueicon : mdl.redicon) : mdl.ffaicon;
                g.text("", 0, icon);
                if((o==player1 && highlightscore && (multiplayer(false) || demoplayback || players.length() > 1)) || bgcolor > -1) g.poplist(); //NEW || bgcolor > -1
            });
            g.poplist();

            if(sg.team && m_teammode)
            {
                g.pushlist(); // vertical

                if(sg.score>=10000) g.textf("%s: WIN", fgcolor, NULL, sg.team);
                else g.textf("%s: %d", fgcolor, NULL, sg.team, sg.score);

                g.pushlist(); // horizontal
            }

            if(!cmode || !cmode->hidefrags() || !hidefrags || displayextinfo(showfrags)) //NEW || displayextinfo(showfrags)
            { 
                g.pushlist();
                g.strut(6);
                g.text("frags", fgcolor);
                //NEW
                loopscoregroup(o,
                {
                    if(hidefrags && o->flags) g.textf("%d/%d", scoreboardtextcolor, NULL, o->frags, o->flags);
                    else g.textf("%d", scoreboardtextcolor, NULL, o->frags);
                });
                //NEW END
                g.poplist();
            }

            if(oldscoreboard) goto next; //NEW
            name:; //NEW

            if(oldscoreboard) g.space(6); //NEW
            g.pushlist();
            g.text("name", fgcolor);
            g.strut(12);
            loopscoregroup(o,
            {
                g.textf("%s ", statuscolor(o, scoreboardtextcolor), NULL, colorname(o));           //NEW scoreboardtextcolor   instead of   0xFFFFDD)
            });
            g.poplist();

            if(oldscoreboard) goto cn;
            next:; //NEW

            //NEW
            if(displayextinfo<bool>())
            {
                if(showdeaths)
                {
                    g.pushlist();
                    g.strut(6);
                    g.text("deaths", fgcolor);

                    loopscoregroup(o,
                    {
                        if(o->extinfo) g.textf("%d", scoreboardtextcolor, NULL, o->extinfo->deaths);
                        else g.text("??", scoreboardtextcolor);
                    });

                    g.poplist();
                }

                if(showdamagedealt)
                {
                    // More or less the same as
                    // https://github.com/sauerworld/community-edition/blob/2e31507b5d5/fpsgame/scoreboard.cpp#L314

                    g.pushlist();
                    g.strut(6);
                    g.text("dd", fgcolor);

                    loopscoregroup(o,
                    {
                        bool get = o->damagedealt>=1000 || showdamagedealt==2;
                        int damagedealt;
                        if(o->extinfo && (o->extinfo->ext.ishopmodcompatible() || o->extinfo->ext.isoomod())) damagedealt = max(o->damagedealt, o->extinfo->ext.damage);
                        else damagedealt = o->damagedealt;
                        g.textf(get ? "%.2fk" : "%.0f", scoreboardtextcolor, NULL, get ? damagedealt/1000.f : damagedealt*1.f));
                    }

                    g.poplist();
                }

                if(showkpd)
                {
                    g.pushlist();
                    g.strut(6);
                    g.text("kpd", fgcolor);

                    loopscoregroup(o,
                    {
                        if(o->extinfo) g.textf("%.2f", scoreboardtextcolor, NULL, o->extinfo->frags / (o->extinfo->deaths > 0 ? o->extinfo->deaths*1.f : 1.00f));
                        else g.text("??", scoreboardtextcolor);
                    });

                    g.poplist();
                }

                if(showacc)
                {
                    g.pushlist();
                    g.strut(6);
                    g.text("acc", fgcolor);

                    loopscoregroup(o,
                    {
                        if(o->extinfo) g.textf("%d%%", scoreboardtextcolor, NULL, o->extinfo->acc);
                        else g.text("??", scoreboardtextcolor);
                    });

                    g.poplist();
                }

                if(showtks && m_teammode)
                {
                    g.pushlist();
                    g.strut(8);
                    g.text("teamkills", fgcolor);

                    string tks;
                    loopscoregroup(o, g.text(gamemod::calcteamkills(o, tks, sizeof(tks)) ? tks : "??", scoreboardtextcolor, NULL));

                    g.poplist();
                }

                if(showip)
                {
                    g.pushlist();
                    g.strut(11);
                    g.text("ip", fgcolor);
                    loopscoregroup(o, renderip(g, o));
                    g.poplist();
                }
            }
            //NEW END

            if(multiplayer(false) || demoplayback)
            {
                if(showpj || showping) g.space(1);

                if(showpj && showping <= 1)
                {
                    g.pushlist();
                    g.strut(6);
                    g.text("pj", fgcolor);
                    loopscoregroup(o,
                    {
                        if(o->state==CS_LAGGED) g.text("LAG", scoreboardtextcolor);
                        else g.textf("%d", scoreboardtextcolor, NULL, o->plag);
                    });
                    g.poplist();
                }

                if(showping > 1)
                {
                    g.pushlist();
                    g.strut(6);

                    g.pushlist();
                    g.text("ping", fgcolor);
                    g.space(1);
                    g.spring();
                    g.text("pj", fgcolor);
                    g.poplist();

                    loopscoregroup(o,
                    {
                        fpsent *p = o->ownernum >= 0 ? getclient(o->ownernum) : o;
                        if(!p) p = o;
                        g.pushlist();
                        if(p->state==CS_LAGGED) g.text("LAG", 0xFFFFDD);
                        else
                        {
                            g.textf("%d", 0xFFFFDD, NULL, p->ping);
                            g.space(1);
                            g.spring();
                            g.textf("%d", 0xFFFFDD, NULL, o->plag);
                        }
                        g.poplist();

                    });
                    g.poplist();
                }
                else if(showping)
                {
                    g.pushlist();
                    g.text("ping", fgcolor);
                    g.strut(6);
                    loopscoregroup(o,
                    {
                        fpsent *p = o->ownernum >= 0 ? getclient(o->ownernum) : o;
                        if(!p) p = o;
                        if(!showpj && p->state==CS_LAGGED) g.text("LAG", scoreboardtextcolor);
                        else g.textf("%d", scoreboardtextcolor, NULL, p->ping);
                    });
                    g.poplist();
                }

                //NEW
                if(displayextinfo(showcountry) && havecountrynames)
                {
                    g.space(2);
                    g.pushlist();
                    g.text("country", fgcolor);
                    g.strut(8);
                    loopscoregroup(o, rendercountry(g, o, showcountry););
                    g.poplist();
                }
                //NEW END
            }

            if(oldscoreboard) goto name; //NEW
            cn:; //NEW

            if(showclientnum==1 || player1->privilege>=PRIV_MASTER) //NEW showclientnum || --> showclientnum==1 ||
            {
                g.space(1);
                g.pushlist();
                g.text("cn", fgcolor);
                loopscoregroup(o, g.textf("%d", scoreboardtextcolor, NULL, o->clientnum));
                g.poplist();
            }

            if(sg.team && m_teammode)
            {
                g.poplist(); // horizontal
                g.poplist(); // vertical
            }

            g.poplist(); // horizontal
            g.poplist(); // vertical

            if(k+1<numgroups && (k+1)%2) g.space(2);
            else g.poplist(); // horizontal
        }

        if(showspectators && spectators.length())
        {
            if(showclientnum || player1->privilege>=PRIV_MASTER)
            {
                g.pushlist();

                g.pushlist();
                g.text("spectator", scoreboardtextcolorhead, " ");
                //g.strut(12); //NEW commented
                loopv(spectators) 
                {
                    fpsent *o = spectators[i];
                    int bgcolor = gamemod::guiplayerbgcolor(o, getserveraddress());          //NEW
                    if((o==player1 && highlightscore) || bgcolor > -1)                       //NEW || bgcolor > -1
                    {
                        g.pushlist();
                        g.background(bgcolor > -1 ? bgcolor : 0x808080, 3);                  //NEW bgcolor > -1 ? bgcolor :
                    }
                    g.text(colorname(o), statuscolor(o, scoreboardtextcolor), "spectator");  //NEW scoreboardtextcolor   instead of   0xFFFFDD
                    if((o==player1 && highlightscore) || bgcolor > -1) g.poplist();          //NEW || bgcolor > -1
                }
                g.poplist();

#if 0 //NEW commented
                if((multiplayer(false) || demoplayback) && showspectatorping)
                {
                    g.space(1);
                    g.pushlist();
                    g.text("ping", 0xFFFF80);
                    g.strut(6);
                    loopv(spectators)
                    {
                        fpsent *o = spectators[i];
                        fpsent *p = o->ownernum >= 0 ? getclient(o->ownernum) : o;
                        if(!p) p = o;
                        if(p->state==CS_LAGGED) g.text("LAG", 0xFFFFDD);
                        else g.textf("%d", 0xFFFFDD, NULL, p->ping);
                    }
                    g.poplist();
                }
#endif

                if(showclientnum==1) //NEW
                {
                    g.space(1);
                    g.pushlist();
                    g.text("cn", scoreboardtextcolorhead);
                    loopv(spectators) g.textf("%d", scoreboardtextcolor, NULL, spectators[i]->clientnum);
                    g.poplist();
                }

                //NEW
                if(showspectatorping)
                {
                    g.space(1);
                    g.pushlist();
                    g.text("ping", scoreboardtextcolorhead);
                    loopv(spectators) g.textf("%d", scoreboardtextcolor, NULL, spectators[i]->ping);
                    g.poplist();
                }

                if(displayextinfo(showspectatorip))
                {
                    g.space(1);
                    g.pushlist();
                    g.text("ip", scoreboardtextcolorhead);
                    loopv(spectators) renderip(g, spectators[i]);
                    g.poplist();
                }

                if(displayextinfo(showcountry) && havecountrynames)
                {
                    g.space(1);
                    g.pushlist();
                    g.text("country", scoreboardtextcolorhead);
                    loopv(spectators) rendercountry(g, spectators[i], showcountry);
                    g.poplist();
                }
                //NEW END

                g.poplist();
            }
            else
            {
                g.textf("%d spectator%s", scoreboardtextcolorhead, " ", spectators.length(), spectators.length()!=1 ? "s" : "");
                loopv(spectators)
                {
                    if((i%3)==0)
                    {
                        g.pushlist();
                        g.text("", scoreboardtextcolor, "spectator");
                    }
                    fpsent *o = spectators[i];
                    int status = scoreboardtextcolor;
                    int bgcolor = gamemod::guiplayerbgcolor(o, getserveraddress());                                  //NEW
                    if(bgcolor > -1) status = bgcolor;                                                               //NEW
                    if((o==player1 && highlightscore) || bgcolor > -1)                                               //NEW || bgcolor > -1
                    {
                        g.pushlist();
                        g.background(bgcolor > -1 ? bgcolor : 0x808080);                                             //NEW bgcolor > -1 ? bgcolor :
                    }
                    g.text(colorname(o), statuscolor(o, status), countryflag(o->extinfo ? o->countrycode : NULL));   //NEW countryflag()   and    status   instead of   0xFFFFDD
                    if((o==player1 && highlightscore) || bgcolor > -1) g.poplist();                                  //NEW || bgcolor > -1) g.poplist();
                    if(i+1<spectators.length() && (i+1)%3) g.space(1);
                    else g.poplist();
                }
            }
        }
    }

    struct scoreboardgui : g3d_callback
    {
        bool showing;
        vec menupos;
        int menustart;

        scoreboardgui() : showing(false) {}

        void show(bool on)
        {
            if(!showing && on)
            {
                menupos = menuinfrontofplayer();
                menustart = starttime();
            }
            showing = on;
        }

        void gui(g3d_gui &g, bool firstpass)
        {
            g.start(menustart, 0.03f, NULL, false);
            renderscoreboard(g, firstpass);
            g.end();
        }

        void render()
        {
            if(showing) g3d_addgui(this, menupos, (scoreboard2d ? GUI_FORCE_2D : GUI_2D | GUI_FOLLOW) | GUI_BOTTOM);
        }

    } scoreboard;

    void g3d_gamemenus()
    {
        scoreboard.render();
    }

    VARFN(scoreboard, showscoreboard, 0, 0, 1, scoreboard.show(showscoreboard!=0));

    bool isscoreboardshown()
    {
        return showscoreboard != 0; //NEW
    }

    void showscores(bool on)
    {
        showscoreboard = on ? 1 : 0;
        scoreboard.show(on);
    }
    ICOMMAND(showscores, "D", (int *down), showscores(*down!=0));

    VARP(hudscore, 0, 0, 1);
    FVARP(hudscorescale, 1e-3f, 1.0f, 1e3f);
    VARP(hudscorealign, -1, 0, 1);
    FVARP(hudscorex, 0, 0.50f, 1);
    FVARP(hudscorey, 0, 0.03f, 1);
    HVARP(hudscoreplayercolour, 0, 0x60A0FF, 0xFFFFFF);
    HVARP(hudscoreenemycolour, 0, 0xFF4040, 0xFFFFFF);
    VARP(hudscorealpha, 0, 255, 255);
    VARP(hudscoresep, 0, 200, 1000);

    void drawhudscore(int w, int h)
    {
        int numgroups = groupplayers();
        if(!numgroups) return;

        scoregroup *g = groups[0];
        int score = INT_MIN, score2 = INT_MIN;
        bool best = false;
        if(m_teammode)
        {
            score = g->score;
            best = isteam(player1->team, g->team);
            if(numgroups > 1)
            {
                if(best) score2 = groups[1]->score;
                else for(int i = 1; i < groups.length(); ++i) if(isteam(player1->team, groups[i]->team)) { score2 = groups[i]->score; break; }
                if(score2 == INT_MIN)
                {
                    fpsent *p = followingplayer(player1);
                    if(p->state==CS_SPECTATOR) score2 = groups[1]->score;
                }
            }
        }
        else
        {
            fpsent *p = followingplayer(player1);
            score = g->players[0]->frags;
            best = p == g->players[0];
            if(g->players.length() > 1)
            {
                if(best || p->state==CS_SPECTATOR) score2 = g->players[1]->frags;
                else score2 = p->frags;
            }
        }
        if(score == score2 && !best) best = true;

        score = clamp(score, -999, 9999);
        defformatstring(buf, "%d", score);
        int tw = 0, th = 0;
        text_bounds(buf, tw, th);

        string buf2;
        int tw2 = 0, th2 = 0;
        if(score2 > INT_MIN)
        {
            score2 = clamp(score2, -999, 9999);
            formatstring(buf2, "%d", score2);
            text_bounds(buf2, tw2, th2);
        }

        int fw = 0, fh = 0;
        text_bounds("00", fw, fh);
        fw = max(fw, max(tw, tw2));

        vec2 offset = vec2(hudscorex, hudscorey).mul(vec2(w, h).div(hudscorescale));
        if(hudscorealign == 1) offset.x -= 2*fw + hudscoresep;
        else if(hudscorealign == 0) offset.x -= (2*fw + hudscoresep) / 2.0f;
        vec2 offset2 = offset;
        offset.x += (fw-tw)/2.0f;
        offset.y -= th/2.0f;
        offset2.x += fw + hudscoresep + (fw-tw2)/2.0f;
        offset2.y -= th2/2.0f;

        pushhudmatrix();
        hudmatrix.scale(hudscorescale, hudscorescale, 1);
        flushhudmatrix();

        int color = hudscoreplayercolour, color2 = hudscoreenemycolour;
        if(!best) swap(color, color2);

        draw_text(buf, int(offset.x), int(offset.y), (color>>16)&0xFF, (color>>8)&0xFF, color&0xFF, hudscorealpha);
        if(score2 > INT_MIN) draw_text(buf2, int(offset2.x), int(offset2.y), (color2>>16)&0xFF, (color2>>8)&0xFF, color2&0xFF, hudscorealpha);

        pophudmatrix();
    }
}

