#include "game.h"

namespace game
{
    bool intermission = false;
    int maptime = 0, maprealtime = 0, maplimit = -1;
    int respawnent = -1;
    int lasthit = 0, lastspawnattempt = 0;

    int following = -1, followdir = 0;

    fpsent *player1 = NULL;         // our client
    vector<fpsent *> players;       // other clients
    int savedammo[NUMGUNS];

    bool clientoption(const char *arg) { return false; }

    void taunt()
    {
        if(player1->state!=CS_ALIVE || player1->physstate<PHYS_SLOPE) return;
        if(lastmillis-player1->lasttaunt<1000) return;
        player1->lasttaunt = lastmillis;
        addmsg(N_TAUNT, "rc", player1);
    }
    COMMAND(taunt, "");

    ICOMMAND(getfollow, "", (),
    {
        fpsent *f = followingplayer();
        intret(f ? f->clientnum : -1);
    });

	void follow(char *arg)
    {
        if(arg[0] ? player1->state==CS_SPECTATOR : following>=0)
        {
            following = arg[0] ? parseplayer(arg) : -1;
            if(following==player1->clientnum) following = -1;
            followdir = 0;
            conoutf("follow %s", following>=0 ? "on" : "off");
        }
	}
    COMMAND(follow, "s");

    void nextfollow(int dir)
    {
        if(player1->state!=CS_SPECTATOR || clients.empty())
        {
            stopfollowing();
            return;
        }
        int cur = following >= 0 ? following : (dir > 0 ? clients.length() - 1 : 0);
        loopv(clients)
        {
            cur = (cur + dir + clients.length()) % clients.length();
            if(clients[cur] && clients[cur]->state!=CS_SPECTATOR)
            {
                if(following<0) conoutf("follow on");
                following = cur;
                followdir = dir;
                return;
            }
        }
        stopfollowing();
    }
    ICOMMAND(nextfollow, "i", (int *dir), nextfollow(*dir < 0 ? -1 : 1));


    const char *getclientmap() { return clientmap; }

    void resetgamestate()
    {
        if(m_classicsp)
        {
            clearmovables();
            clearmonsters();                 // all monsters back at their spawns for editing
            entities::resettriggers();
        }
        clearprojectiles();
        clearbouncers();
    }

    fpsent *spawnstate(fpsent *d)              // reset player state not persistent accross spawns
    {
        d->respawn();
        d->spawnstate(gamemode);
        return d;
    }

    void respawnself()
    {
        if(ispaused()) return;
        if(m_mp(gamemode))
        {
            int seq = (player1->lifesequence<<16)|((lastmillis/1000)&0xFFFF);
            if(player1->respawned!=seq) { addmsg(N_TRYSPAWN, "rc", player1); player1->respawned = seq; }
        }
        else
        {
            spawnplayer(player1);
            showscores(false);
            lasthit = 0;
            if(cmode) cmode->respawned(player1);
        }
    }

    fpsent *pointatplayer()
    {
        loopv(players) if(players[i] != player1 && intersect(players[i], player1->o, worldpos)) return players[i];
        return NULL;
    }

    void stopfollowing()
    {
        if(following<0) return;
        following = -1;
        followdir = 0;
        conoutf("follow off");
    }

    fpsent *followingplayer(fpsent *fallback)
    {
        if(player1->state!=CS_SPECTATOR || following<0) return fallback;
        fpsent *target = getclient(following);
        if(target && target->state!=CS_SPECTATOR) return target;
        return fallback;
    }

    fpsent *hudplayer()
    {
        if(thirdperson && allowthirdperson()) return player1;
        return followingplayer(player1);
    }

    void setupcamera()
    {
        fpsent *target = followingplayer();
        if(target)
        {
            player1->yaw = target->yaw;
            player1->pitch = target->state==CS_DEAD ? 0 : target->pitch;
            player1->o = target->o;
            player1->resetinterp();
        }
    }

    bool allowthirdperson(bool msg)
    {
        return player1->state==CS_SPECTATOR || player1->state==CS_EDITING || m_edit || !multiplayer(msg);
    }
    ICOMMAND(allowthirdperson, "b", (int *msg), intret(allowthirdperson(*msg!=0) ? 1 : 0));

    bool detachcamera()
    {
        fpsent *d = hudplayer();
        return d->state==CS_DEAD;
    }

    bool collidecamera()
    {
        switch(player1->state)
        {
            case CS_EDITING: return false;
            case CS_SPECTATOR: return followingplayer()!=NULL;
        }
        return true;
    }

    VARP(smoothmove, 0, 75, 100);
    VARP(smoothdist, 0, 32, 64);

    void predictplayer(fpsent *d, bool move)
    {
        d->o = d->newpos;
        d->yaw = d->newyaw;
        d->pitch = d->newpitch;
        d->roll = d->newroll;
        if(move)
        {
            moveplayer(d, 1, false);
            d->newpos = d->o;
        }
        float k = 1.0f - float(lastmillis - d->smoothmillis)/smoothmove;
        if(k>0)
        {
            d->o.add(vec(d->deltapos).mul(k));
            d->yaw += d->deltayaw*k;
            if(d->yaw<0) d->yaw += 360;
            else if(d->yaw>=360) d->yaw -= 360;
            d->pitch += d->deltapitch*k;
            d->roll += d->deltaroll*k;
        }
    }

    void otherplayers(int curtime)
    {
        loopv(players)
        {
            fpsent *d = players[i];
            if(d == player1 || d->ai) continue;

            if(d->state==CS_DEAD && d->ragdoll) moveragdoll(d);
            else if(!intermission)
            {
                if(lastmillis - d->lastaction >= d->gunwait) d->gunwait = 0;
                if(d->quadmillis) entities::checkquad(curtime, d);
            }

            const int lagtime = totalmillis-d->lastupdate;
            if(!lagtime || intermission) continue;
            else if(lagtime>1000 && d->state==CS_ALIVE)
            {
                d->state = CS_LAGGED;
                continue;
            }
            if(d->state==CS_ALIVE || d->state==CS_EDITING)
            {
                if(smoothmove && d->smoothmillis>0) predictplayer(d, true);
                else moveplayer(d, 1, false);
            }
            else if(d->state==CS_DEAD && !d->ragdoll && lastmillis-d->lastpain<2000) moveplayer(d, 1, true);
        }
    }

    VARFP(slowmosp, 0, 0, 1, { if(m_sp && !slowmosp) server::forcegamespeed(100); });

    void checkslowmo()
    {
        static int lastslowmohealth = 0;
        server::forcegamespeed(intermission ? 100 : clamp(player1->health, 25, 200));
        if(player1->health<player1->maxhealth && lastmillis-max(maptime, lastslowmohealth)>player1->health*player1->health/2)
        {
            lastslowmohealth = lastmillis;
            player1->health++;
        }
    }

    void updateworld()        // main game update loop
    {
        if(!maptime) { maptime = lastmillis; maprealtime = totalmillis; return; }
        if(!curtime) { gets2c(); if(player1->clientnum>=0) c2sinfo(); return; }

        physicsframe();
        ai::navigate();
        if(player1->state != CS_DEAD && !intermission)
        {
            if(player1->quadmillis) entities::checkquad(curtime, player1);
        }
        updateweapons(curtime);
        otherplayers(curtime);
        ai::update();
        moveragdolls();
        gets2c();
        updatemovables(curtime);
        updatemonsters(curtime);
        if(connected)
        {
            if(player1->state == CS_DEAD)
            {
                if(player1->ragdoll) moveragdoll(player1);
                else if(lastmillis-player1->lastpain<2000)
                {
                    player1->move = player1->strafe = 0;
                    moveplayer(player1, 10, true);
                }
            }
            else if(!intermission)
            {
                if(player1->ragdoll) cleanragdoll(player1);
                moveplayer(player1, 10, true);
                swayhudgun(curtime);
                entities::checkitems(player1);
                if(m_sp)
                {
                    if(slowmosp) checkslowmo();
                    if(m_classicsp) entities::checktriggers();
                }
                else if(cmode) cmode->checkitems(player1);
            }
        }
        if(player1->clientnum>=0) c2sinfo();   // do this last, to reduce the effective frame lag
    }

    float proximityscore(float x, float lower, float upper)
    {
        if(x <= lower) return 1.0f;
        if(x >= upper) return 0.0f;
        float a = x - lower, b = x - upper;
        return (b * b) / (a * a + b * b);
    }

    static inline float harmonicmean(float a, float b) { return a + b > 0 ? 2 * a * b / (a + b) : 0.0f; }

    // avoid spawning near other players
    float ratespawn(dynent *d, const extentity &e)
    {
        fpsent *p = (fpsent *)d;
        vec loc = vec(e.o).addz(p->eyeheight);
        float maxrange = !m_noitems ? 400.0f : (cmode ? 300.0f : 110.0f);
        float minplayerdist = maxrange;
        loopv(players)
        {
            const fpsent *o = players[i];
            if(o == p)
            {
                if(m_noitems || (o->state != CS_ALIVE && lastmillis - o->lastpain > 3000)) continue;
            }
            else if(o->state != CS_ALIVE || isteam(o->team, p->team)) continue;

            vec dir = vec(o->o).sub(loc);
            float dist = dir.squaredlen();
            if(dist >= minplayerdist*minplayerdist) continue;
            dist = sqrtf(dist);
            dir.mul(1/dist);

            // scale actual distance if not in line of sight
            if(raycube(loc, dir, dist) < dist) dist *= 1.5f;
            minplayerdist = min(minplayerdist, dist);
        }
        float rating = 1.0f - proximityscore(minplayerdist, 80.0f, maxrange);
        return cmode ? harmonicmean(rating, cmode->ratespawn(p, e)) : rating;
    }

    void pickgamespawn(fpsent *d)
    {
        int ent = m_classicsp && d == player1 && respawnent >= 0 ? respawnent : -1;
        int tag = cmode ? cmode->getspawngroup(d) : 0;
        findplayerspawn(d, ent, tag);
    }

    void spawnplayer(fpsent *d)   // place at random spawn
    {
        pickgamespawn(d);
        spawnstate(d);
        if(d==player1)
        {
            if(editmode) d->state = CS_EDITING;
            else if(d->state != CS_SPECTATOR) d->state = CS_ALIVE;
        }
        else d->state = CS_ALIVE;
    }

    VARP(spawnwait, 0, 0, 1000);

    void respawn()
    {
        if(player1->state==CS_DEAD)
        {
            player1->attacking = false;
            int wait = cmode ? cmode->respawnwait(player1) : 0;
            if(wait>0)
            {
                lastspawnattempt = lastmillis;
                //conoutf(CON_GAMEINFO, "\f2you must wait %d second%s before respawn!", wait, wait!=1 ? "s" : "");
                return;
            }
            if(lastmillis < player1->lastpain + spawnwait) return;
            if(m_dmsp) { changemap(clientmap, gamemode); return; }    // if we die in SP we try the same map again
            respawnself();
            if(m_classicsp)
            {
                conoutf(CON_GAMEINFO, "\f2You wasted another life! The monsters stole your armour and some ammo...");
                loopi(NUMGUNS) if(i!=GUN_PISTOL && (player1->ammo[i] = savedammo[i]) > 5) player1->ammo[i] = max(player1->ammo[i]/3, 5);
            }
        }
    }
    COMMAND(respawn, "");

    // inputs

    VARP(attackspawn, 0, 1, 1);

    void doattack(bool on)
    {
        if(!connected || intermission) return;
        if((player1->attacking = on) && attackspawn) respawn();
    }

    VARP(jumpspawn, 0, 1, 1);

    bool canjump()
    {
        if(!connected || intermission) return false;
        if(jumpspawn) respawn();
        return player1->state!=CS_DEAD;
    }

    bool allowmove(physent *d)
    {
        if(d->type!=ENT_PLAYER) return true;
        return !((fpsent *)d)->lasttaunt || lastmillis-((fpsent *)d)->lasttaunt>=1000;
    }

    VARP(hitsound, 0, 0, 1);

    void damaged(int damage, fpsent *d, fpsent *actor, bool local)
    {
        if((d->state!=CS_ALIVE && d->state != CS_LAGGED && d->state != CS_SPAWNING) || intermission) return;

        if(local) damage = d->dodamage(damage);
        else if(actor==player1) return;

        fpsent *h = hudplayer();
        if(h!=player1 && actor==h && d!=actor)
        {
            if(hitsound && lasthit != lastmillis) playsound(S_HIT);
            lasthit = lastmillis;
        }
        if(d==h)
        {
            damageblend(damage);
            damagecompass(damage, actor->o);
        }
        damageeffect(damage, d, d!=h);

		ai::damaged(d, actor);

        if(m_sp && slowmosp && d==player1 && d->health < 1) d->health = 1;

        if(d->health<=0) { if(local) killed(d, actor); }
        else if(d==h) playsound(S_PAIN6);
        else playsound(S_PAIN1+rnd(5), &d->o);
    }

    VARP(deathscore, 0, 1, 1);

    void deathstate(fpsent *d, bool restore)
    {
        d->state = CS_DEAD;
        d->lastpain = lastmillis;
        if(!restore)
        {
            gibeffect(max(-d->health, 0), d->vel, d);
            d->deaths++;
        }
        if(d==player1)
        {
            if(deathscore) showscores(true);
            disablezoom();
            if(!restore) loopi(NUMGUNS) savedammo[i] = player1->ammo[i];
            d->attacking = false;
            //d->pitch = 0;
            d->roll = 0;
            playsound(S_DIE1+rnd(2));
        }
        else
        {
            d->move = d->strafe = 0;
            d->resetinterp();
            d->smoothmillis = 0;
            playsound(S_DIE1+rnd(2), &d->o);
        }
    }

    VARP(teamcolorfrags, 0, 1, 1);
    MODVARP(indentfrags, 0, 4, 8); //NEW

    void killed(fpsent *d, fpsent *actor)
    {
        if(d->state==CS_EDITING)
        {
            d->editstate = CS_DEAD;
            d->deaths++;
            if(d!=player1) d->resetinterp();
            return;
        }
        else if((d->state!=CS_ALIVE && d->state != CS_LAGGED && d->state != CS_SPAWNING) || intermission) return;

        if(mod::event::run(mod::event::PLAYER_FRAG, "dsdsd", actor->clientnum, actor->name, d->clientnum, d->name, actor->gunselect) <= 0) //NEW
        {
            if(cmode) cmode->died(d, actor);

            fpsent *h = followingplayer(player1);
            int contype = d==h || actor==h ? CON_FRAG_SELF : CON_FRAG_OTHER;
            const char *dname = "", *aname = "";
            if(m_teammode && teamcolorfrags)
            {
                dname = teamcolorname(d, "you");
                aname = teamcolorname(actor, "you");
            }
            else
            {
                dname = colorname(d, NULL, "", "", "you");
                aname = colorname(actor, NULL, "", "", "you");
            }
            string indent;                                              //NEW
            mod::strtool is(indent, sizeof(indent));                    //NEW
            is.add(' ', indentfrags);                                   //NEW
            is.finalize();                                              //NEW
            if(actor->type==ENT_AI)
                conoutf(contype, "%s\f2%s got killed by %s!", indent, dname, aname);          //NEW indent
            else if(d==actor || actor->type==ENT_INANIMATE)
                conoutf(contype, "%s\f2%s suicided%s", indent, dname, d==player1 ? "!" : ""); //NEW indent
            else if(isteam(d->team, actor->team))
            {
                //NEW
                string tkstats; tkstats[0] = '\0';
                gamemod::countteamkills(actor, true, tkstats, sizeof(tkstats));
                mod::event::run(mod::event::PLAYER_TEAM_KILL, "dsds", actor->clientnum, actor->name, d->clientnum, d->name);
                //NEW END
                contype |= CON_TEAMKILL;
                if(actor==player1) conoutf(contype, "%s\f6%s fragged a teammate (%s)", indent, aname, dname);            //NEW indent
                else if(d==player1) conoutf(contype, "%s\f6%s got fragged by a teammate (%s)", indent, dname, aname);    //NEW indent
                else conoutf(contype, "%s\f6%s fragged a teammate (%s) %s", indent, aname, dname, tkstats);              //NEW indent, tkstats
            }
            else
            {
                if(d==player1) conoutf(contype, "%s\f2%s got fragged by %s", indent, dname, aname); //NEW indent
                else conoutf(contype, "%s\f2%s fragged %s", indent, aname, dname);                  //NEW indent
            }
        }
        deathstate(d);
		ai::killed(d, actor);
    }

    void timeupdate(int secs)
    {
        server::timeupdate(secs);
        if(secs > 0)
        {
            maplimit = lastmillis + secs*1000;
        }
        else
        {
            intermission = true;
            player1->attacking = false;
            if(cmode) cmode->gameover();
            conoutf(CON_GAMEINFO, "\f2intermission:");
            conoutf(CON_GAMEINFO, "\f2game has ended!");
            if(m_ctf) conoutf(CON_GAMEINFO, "\f2player frags: %d, flags: %d, deaths: %d", player1->frags, player1->flags, player1->deaths);
            else if(m_collect) conoutf(CON_GAMEINFO, "\f2player frags: %d, skulls: %d, deaths: %d", player1->frags, player1->flags, player1->deaths);
            else conoutf(CON_GAMEINFO, "\f2player frags: %d, deaths: %d", player1->frags, player1->deaths);
            int accuracy = (player1->totaldamage*100)/max(player1->totalshots, 1);
            conoutf(CON_GAMEINFO, "\f2player total damage dealt: %d, damage wasted: %d, accuracy(%%): %d", player1->totaldamage, player1->totalshots-player1->totaldamage, accuracy);
            if(m_sp) spsummary(accuracy);

            showscores(true);
            disablezoom();

            execident("intermission");
            mod::event::run(mod::event::INTERMISSION, "ss", server::modename(gamemode), game::getclientmap()); //NEW
        }
    }

    //NEW
    void adjusttimeleft(int millis)
    {
        lastmillis += millis;
    }

    int gettimeleft()
    {
        return maplimit-lastmillis;
    }

    fpsent *getclient(const char *client)
    {
        if (*client)
        {
            char *end = NULL;
            int cn = strtol(client, &end, 10);
            if (end > client && !*end) return getclient(cn);
        }
        return NULL;
    }
    //NEW END

    //NEW extended commands to support other clients
    ICOMMAND(getfrags, "s", (const char *cn), fpsent *o = getclient(cn); if(!o) o = player1; intret(o->frags));
    ICOMMAND(getflags, "s", (const char *cn), fpsent *o = getclient(cn); if(!o) o = player1; intret(o->flags));
    ICOMMAND(getdeaths, "s", (const char *cn), fpsent *o = getclient(cn); if(!o) o = player1; intret(o->extinfo ? o->extinfo->deaths : o->deaths));
    ICOMMAND(getteamkills, "s", (const char *cn), fpsent *o = getclient(cn); if(!o) o = player1; intret(o->extinfo ? o->extinfo->teamkills : o->teamkills)); //NEW
    ICOMMAND(getaccuracy, "s", (const char *cn), fpsent *o = getclient(cn); if(!o) o = player1; intret(o->extinfo ? o->extinfo->acc : (o->totaldamage*100)/max(o->totalshots, 1)));
    ICOMMAND(gettotaldamage, "s", (const char *cn), fpsent *o = getclient(cn); if(!o) o = player1; intret(o->totaldamage));
    ICOMMAND(gettotalshots, "s", (const char *cn), fpsent *o = getclient(cn); if(!o) o = player1; intret(o->totalshots));
    ICOMMAND(getping, "s", (const char *cn), fpsent *o = getclient(cn); if(!o) o = player1; intret(o->ping)); //NEW
    ICOMMAND(getfloatping, "s", (const char *cn), fpsent *o = getclient(cn); if(!o) o = player1; floatret(o->highresping ? o->highresping : (float)o->ping)); //NEW
    ICOMMAND(gethudclientnum, "", (), intret(hudplayer()->clientnum)); //NEW

    vector<fpsent *> clients;

    fpsent *newclient(int cn)   // ensure valid entity
    {
        if(cn < 0 || cn > max(0xFF, MAXCLIENTS + MAXBOTS))
        {
            neterr("clientnum", false);
            return NULL;
        }

        if(cn == player1->clientnum) return player1;

        while(cn >= clients.length()) clients.add(NULL);
        if(!clients[cn])
        {
            fpsent *d = new fpsent;
            d->clientnum = cn;
            clients[cn] = d;
            players.add(d);
            mod::extinfo::newplayer(d->clientnum); //NEW
        }
        return clients[cn];
    }

    fpsent *getclient(int cn)   // ensure valid entity
    {
        if(cn == player1->clientnum)
        {
            if(game::demoplayback) return NULL; //NEW  don't allow demos to get player1
            return player1;
        }
        return clients.inrange(cn) ? clients[cn] : NULL;
    }

    void clientdisconnected(int cn, bool notify)
    {
        if(!clients.inrange(cn)) return;
        if(following==cn)
        {
            if(followdir) nextfollow(followdir);
            else stopfollowing();
        }
        unignore(cn);
        fpsent *d = clients[cn];
        if(!d) return;
        if(notify && d->name[0])
        {
            mod::event::run(mod::event::PLAYER_DISCONNECT, "ds", d->clientnum, d->name); //NEW
            conoutf("\f4leave:\f7 %s", colorname(d));
        }
        removeweapons(d);
        removetrackedparticles(d);
        removetrackeddynlights(d);
        if(cmode) cmode->removeplayer(d);
        players.removeobj(d);
        DELETEP(clients[cn]);
        cleardynentcache();
    }

    void clearclients(bool notify)
    {
        loopv(clients) if(clients[i]) clientdisconnected(i, notify);
    }

    void initclient()
    {
        player1 = spawnstate(new fpsent);
        filtertext(player1->name, "unnamed", false, false, MAXNAMELEN);
        players.add(player1);
    }

    VARP(showmodeinfo, 0, 1, 1);

    void startgame()
    {
        clearmovables();
        clearmonsters();

        clearprojectiles();
        clearbouncers();
        clearragdolls();

        clearteaminfo();

        // reset perma-state
        loopv(players)
        {
            fpsent *d = players[i];
            d->frags = d->flags = 0;
            d->deaths = 0;
            d->totaldamage = 0;
            d->damagedealt = 0; //NEW
            d->totalshots = 0;
            d->maxhealth = 100;
            d->lifesequence = -1;
            d->respawned = d->suicided = -2;
            d->teamkills = 0;
        }

        setclientmode();

        intermission = false;
        maptime = maprealtime = 0;
        maplimit = -1;

        if(cmode)
        {
            cmode->preload();
            cmode->setup();
        }

        conoutf(CON_GAMEINFO, "\f2game mode is %s", server::modename(gamemode));

        if(m_sp)
        {
            defformatstring(scorename, "bestscore_%s", getclientmap());
            const char *best = getalias(scorename);
            if(*best) conoutf(CON_GAMEINFO, "\f2try to beat your best score so far: %s", best);
        }
        else
        {
            const char *info = m_valid(gamemode) ? gamemodes[gamemode - STARTGAMEMODE].info : NULL;
            if(showmodeinfo && info) conoutf(CON_GAMEINFO, "\f0%s", info);
        }

        if(player1->playermodel != playermodel) switchplayermodel(playermodel);

        showscores(false);
        disablezoom();
        lasthit = 0;

        execident("mapstart");
        mod::event::run(mod::event::MAPSTART, "ss", server::modename(gamemode), game::getclientmap()); //NEW
    }

    void loadingmap(const char *name)
    {
        execident("playsong");
    }

    void startmap(const char *name)   // called just after a map load
    {
        ai::savewaypoints();
        ai::clearwaypoints(true);

        respawnent = -1; // so we don't respawn at an old spot
        if(!m_mp(gamemode)) spawnplayer(player1);
        else findplayerspawn(player1, -1);
        entities::resetspawns();
        copystring(clientmap, name ? name : "");

        sendmapinfo();
    }

    const char *getmapinfo()
    {
        return showmodeinfo && m_valid(gamemode) ? gamemodes[gamemode - STARTGAMEMODE].info : NULL;
    }

    const char *getscreenshotinfo()
    {
        return server::modename(gamemode, NULL);
    }

    void physicstrigger(physent *d, bool local, int floorlevel, int waterlevel, int material)
    {
        if(d->type==ENT_INANIMATE) return;
        if     (waterlevel>0) { if(material!=MAT_LAVA) playsound(S_SPLASH1, d==player1 ? NULL : &d->o); }
        else if(waterlevel<0) playsound(material==MAT_LAVA ? S_BURN : S_SPLASH2, d==player1 ? NULL : &d->o);
        if     (floorlevel>0) { if(d==player1 || d->type!=ENT_PLAYER || ((fpsent *)d)->ai) msgsound(S_JUMP, d); }
        else if(floorlevel<0) { if(d==player1 || d->type!=ENT_PLAYER || ((fpsent *)d)->ai) msgsound(S_LAND, d); }
    }

    void dynentcollide(physent *d, physent *o, const vec &dir)
    {
        switch(d->type)
        {
            case ENT_AI: if(dir.z > 0) stackmonster((monster *)d, o); break;
            case ENT_INANIMATE: if(dir.z > 0) stackmovable((movable *)d, o); break;
        }
    }

    void msgsound(int n, physent *d)
    {
        if(!d || d==player1)
        {
            addmsg(N_SOUND, "ci", d, n);
            //NEW
            if(mod::demorecorder::demorecord)
                mod::demorecorder::self::sound(d, n);
            //NEW END
            playsound(n);
        }
        else
        {
            if(d->type==ENT_PLAYER && ((fpsent *)d)->ai)
            {
                addmsg(N_SOUND, "ci", d, n);
                //NEW
                if(mod::demorecorder::demorecord)
                    mod::demorecorder::self::sound(d, n);
                //NEW END
            }
            playsound(n, &d->o);
        }
    }

    int numdynents() { return players.length()+monsters.length()+movables.length(); }

    dynent *iterdynents(int i)
    {
        if(i<players.length()) return players[i];
        i -= players.length();
        if(i<monsters.length()) return (dynent *)monsters[i];
        i -= monsters.length();
        if(i<movables.length()) return (dynent *)movables[i];
        return NULL;
    }

    bool duplicatename(fpsent *d, const char *name = NULL, const char *alt = NULL)
    {
        if(!name) name = d->name;
        if(alt && d != player1 && !strcmp(name, alt)) return true;
        loopv(players) if(d!=players[i] && !strcmp(name, players[i]->name)) return true;
        return false;
    }

    static string cname[3];
    static int cidx = 0;

    const char *colorname(fpsent *d, const char *name, const char *prefix, const char *suffix, const char *alt)
    {
        if(!name) name = alt && d == player1 ? alt : d->name;
        extern int showclientnum; //NEW
        bool dup = showclientnum == 2 || !name[0] || duplicatename(d, name, alt) || d->aitype != AI_NONE; //NEW showclientnum == 2 ||
        if(dup || prefix[0] || suffix[0])
        {
            cidx = (cidx+1)%3;
            if(dup) formatstring(cname[cidx], d->aitype == AI_NONE ? "%s%s \fs\f5(%d)\fr%s" : "%s%s \fs\f5[%d]\fr%s", prefix, name, d->clientnum, suffix);
            else formatstring(cname[cidx], "%s%s%s", prefix, name, suffix);
            return cname[cidx];
        }
        return name;
    }

    VARP(teamcolortext, 0, 1, 1);

    const char *teamcolorname(fpsent *d, const char *alt)
    {
        if(!teamcolortext || !m_teammode || d->state==CS_SPECTATOR) return colorname(d, NULL, "", "", alt);
        return colorname(d, NULL, isteam(d->team, player1->team) ? "\fs\f1" : "\fs\f3", "\fr", alt);
    }

    const char *teamcolor(const char *name, bool sameteam, const char *alt)
    {
        if(!teamcolortext || !m_teammode) return sameteam || !alt ? name : alt;
        cidx = (cidx+1)%3;
        formatstring(cname[cidx], sameteam ? "\fs\f1%s\fr" : "\fs\f3%s\fr", sameteam || !alt ? name : alt);
        return cname[cidx];
    }

    const char *teamcolor(const char *name, const char *team, const char *alt)
    {
        return teamcolor(name, team && isteam(team, player1->team), alt);
    }

    VARP(teamsounds, 0, 1, 1);

    void teamsound(bool sameteam, int n, const vec *loc)
    {
        playsound(n, loc, NULL, teamsounds ? (m_teammode && sameteam ? SND_USE_ALT : SND_NO_ALT) : 0);
    }

    void teamsound(fpsent *d, int n, const vec *loc)
    {
        teamsound(isteam(d->team, player1->team), n, loc);
    }

    void suicide(physent *d)
    {
        if(d==player1 || (d->type==ENT_PLAYER && ((fpsent *)d)->ai))
        {
            if(d->state!=CS_ALIVE) return;
            fpsent *pl = (fpsent *)d;
            if(!m_mp(gamemode)) killed(pl, pl);
            else
            {
                int seq = (pl->lifesequence<<16)|((lastmillis/1000)&0xFFFF);
                if(pl->suicided!=seq) { addmsg(N_SUICIDE, "rc", pl); pl->suicided = seq; }
            }
        }
        else if(d->type==ENT_AI) suicidemonster((monster *)d);
        else if(d->type==ENT_INANIMATE) suicidemovable((movable *)d);
    }
    ICOMMAND(suicide, "", (), suicide(player1));

    bool needminimap() { return m_ctf || m_protect || m_hold || m_capture || m_collect; }

    void drawicon(int icon, float x, float y, float sz)
    {
        settexture("packages/hud/items.png");
        float tsz = 0.25f, tx = tsz*(icon%4), ty = tsz*(icon/4);
        gle::defvertex(2);
        gle::deftexcoord0();
        gle::begin(GL_TRIANGLE_STRIP);
        gle::attribf(x,    y);    gle::attribf(tx,     ty);
        gle::attribf(x+sz, y);    gle::attribf(tx+tsz, ty);
        gle::attribf(x,    y+sz); gle::attribf(tx,     ty+tsz);
        gle::attribf(x+sz, y+sz); gle::attribf(tx+tsz, ty+tsz);
        gle::end();
    }

    float abovegameplayhud(int w, int h)
    {
        switch(hudplayer()->state)
        {
            case CS_EDITING:
            case CS_SPECTATOR:
                return 1;
            default:
                return 1650.0f/1800.0f;
        }
    }

    int ammohudup[3] = { GUN_CG, GUN_RL, GUN_GL },
        ammohuddown[3] = { GUN_RIFLE, GUN_SG, GUN_PISTOL },
        ammohudcycle[7] = { -1, -1, -1, -1, -1, -1, -1 };

    ICOMMAND(ammohudup, "V", (tagval *args, int numargs),
    {
        loopi(3) ammohudup[i] = i < numargs ? getweapon(args[i].getstr()) : -1;
    });

    ICOMMAND(ammohuddown, "V", (tagval *args, int numargs),
    {
        loopi(3) ammohuddown[i] = i < numargs ? getweapon(args[i].getstr()) : -1;
    });

    ICOMMAND(ammohudcycle, "V", (tagval *args, int numargs),
    {
        loopi(7) ammohudcycle[i] = i < numargs ? getweapon(args[i].getstr()) : -1;
    });

    VARP(ammohud, 0, 1, 1);

    void drawammohud(fpsent *d)
    {
        float x = HICON_X + 2*HICON_STEP, y = HICON_Y, sz = HICON_SIZE;
        pushhudmatrix();
        hudmatrix.scale(1/3.2f, 1/3.2f, 1);
        flushhudmatrix();
        float xup = (x+sz)*3.2f, yup = y*3.2f + 0.1f*sz;
        loopi(3)
        {
            int gun = ammohudup[i];
            if(gun < GUN_FIST || gun > GUN_PISTOL || gun == d->gunselect || !d->ammo[gun]) continue;
            drawicon(HICON_FIST+gun, xup, yup, sz);
            yup += sz;
        }
        float xdown = x*3.2f - sz, ydown = (y+sz)*3.2f - 0.1f*sz;
        loopi(3)
        {
            int gun = ammohuddown[3-i-1];
            if(gun < GUN_FIST || gun > GUN_PISTOL || gun == d->gunselect || !d->ammo[gun]) continue;
            ydown -= sz;
            drawicon(HICON_FIST+gun, xdown, ydown, sz);
        }
        int offset = 0, num = 0;
        loopi(7)
        {
            int gun = ammohudcycle[i];
            if(gun < GUN_FIST || gun > GUN_PISTOL) continue;
            if(gun == d->gunselect) offset = i + 1;
            else if(d->ammo[gun]) num++;
        }
        float xcycle = (x+sz/2)*3.2f + 0.5f*num*sz, ycycle = y*3.2f-sz;
        loopi(7)
        {
            int gun = ammohudcycle[(i + offset)%7];
            if(gun < GUN_FIST || gun > GUN_PISTOL || gun == d->gunselect || !d->ammo[gun]) continue;
            xcycle -= sz;
            drawicon(HICON_FIST+gun, xcycle, ycycle, sz);
        }
        pophudmatrix();
    }

    VARP(healthcolors, 0, 1, 1);

    void drawhudicons(fpsent *d)
    {
        pushhudmatrix();
        hudmatrix.scale(2, 2, 1);
        flushhudmatrix();

        defformatstring(health, "%d", d->state==CS_DEAD ? 0 : d->health);
        bvec healthcolor = bvec::hexcolor(healthcolors && !m_insta ? (d->state==CS_DEAD ? 0x808080 : (d->health<=25 ? 0xFF0000 : (d->health<=50 ? 0xFF8000 : (d->health<=100 ? 0xFFFFFF : 0x40C0FF)))) : 0xFFFFFF);
        draw_text(health, (HICON_X + HICON_SIZE + HICON_SPACE)/2, HICON_TEXTY/2, healthcolor.r, healthcolor.g, healthcolor.b);
        if(d->state!=CS_DEAD)
        {
            if(d->armour) draw_textf("%d", (HICON_X + HICON_STEP + HICON_SIZE + HICON_SPACE)/2, HICON_TEXTY/2, d->armour);
            draw_textf("%d", (HICON_X + 2*HICON_STEP + HICON_SIZE + HICON_SPACE)/2, HICON_TEXTY/2, d->ammo[d->gunselect]);
        }

        pophudmatrix();

        if(d->state != CS_DEAD && d->maxhealth > 100)
        {
            float scale = 0.66f;
            pushhudmatrix();
            hudmatrix.scale(scale, scale, 1);
            flushhudmatrix();

            float width, height;
            text_boundsf(health, width, height);
            draw_textf("/%d", (HICON_X + HICON_SIZE + HICON_SPACE + width*2)/scale, (HICON_TEXTY + height)/scale, d->maxhealth);

            pophudmatrix();
        }

        drawicon(HICON_HEALTH, HICON_X, HICON_Y);
        if(d->state!=CS_DEAD)
        {
            if(d->armour) drawicon(HICON_BLUE_ARMOUR+d->armourtype, HICON_X + HICON_STEP, HICON_Y);
            drawicon(HICON_FIST+d->gunselect, HICON_X + 2*HICON_STEP, HICON_Y);
            if(d->quadmillis) drawicon(HICON_QUAD, HICON_X + 3*HICON_STEP, HICON_Y);
            if(ammohud) drawammohud(d);
        }
    }

    bool disableradar = false; // NEW

    VARP(gameclock, 0, 0, 1);
    FVARP(gameclockscale, 0.5, 1.0, 2.0);
    HVARP(gameclockcolour, 0, 0xFFFFFF, 0xFFFFFF);
    VARP(gameclockalpha, 0, 255, 255);
    HVARP(gameclocklowcolour, 0, 0xFFA050, 0xFFFFFF);
    VARP(gameclockalign, -1, 0, 1);
    FVARP(gameclockx, 0.0, 0.50, 1.0);
    FVARP(gameclocky, 0.0, 0.03, 1.0);

    void drawgameclock(int w, int h)
    {
        int secs = max(maplimit-lastmillis + 999, 0)/1000, mins = secs/60;
        secs %= 60;

        defformatstring(buf, "%d:%02d", mins, secs);
        int tw = 0, th = 0;
        text_bounds(buf, tw, th);

        vec2 offset = vec2(gameclockx, gameclocky).mul(vec2(w, h).div(gameclockscale));
        if(gameclockalign == 1) offset.x -= tw;
        else if(gameclockalign == 0) offset.x -= tw/2.0f;
        offset.y -= th/2.0f;

        pushhudmatrix();
        hudmatrix.scale(gameclockscale, gameclockscale, 1);
        flushhudmatrix();

        int color = mins < 1 ? gameclocklowcolour : gameclockcolour;
        draw_text(buf, int(offset.x), int(offset.y), (color>>16)&0xFF, (color>>8)&0xFF, color&0xFF, gameclockalpha);

        pophudmatrix();
    }

    extern int hudscore;
    extern void drawhudscore(int w, int h);

    VARP(ammobar, 0, 0, 1);
    VARP(ammobaralign, -1, 0, 1);
    VARP(ammobarhorizontal, 0, 0, 1);
    VARP(ammobarflip, 0, 0, 1);
    VARP(ammobarhideempty, 0, 1, 1);
    VARP(ammobarsep, 0, 20, 500);
    VARP(ammobarcountsep, 0, 20, 500);
    FVARP(ammobarcountscale, 0.5, 1.5, 2);
    FVARP(ammobarx, 0, 0.025f, 1.0f);
    FVARP(ammobary, 0, 0.5f, 1.0f);
    FVARP(ammobarscale, 0.1f, 0.5f, 1.0f);

    void drawammobarcounter(const vec2 &center, const fpsent *p, int gun)
    {
        vec2 icondrawpos = vec2(center).sub(HICON_SIZE / 2);
        int alpha = p->ammo[gun] ? 0xFF : 0x7F;
        gle::color(bvec(0xFF, 0xFF, 0xFF), alpha);
        drawicon(HICON_FIST + gun, icondrawpos.x, icondrawpos.y);

        int fw, fh; text_bounds("000", fw, fh);
        float labeloffset = HICON_SIZE / 2.0f + ammobarcountsep + ammobarcountscale * (ammobarhorizontal ? fh : fw) / 2.0f;
        vec2 offsetdir = (ammobarhorizontal ? vec2(0, 1) : vec2(1, 0)).mul(ammobarflip ? -1 : 1);
        vec2 labelorigin = vec2(offsetdir).mul(labeloffset).add(center);

        pushhudmatrix();
        hudmatrix.translate(labelorigin.x, labelorigin.y, 0);
        hudmatrix.scale(ammobarcountscale, ammobarcountscale, 1);
        flushhudmatrix();

        defformatstring(label, "%d", p->ammo[gun]);
        int tw, th; text_bounds(label, tw, th);
        vec2 textdrawpos = vec2(-tw, -th).div(2);
        float ammoratio = (float)p->ammo[gun] / itemstats[gun-GUN_SG].add;
        bvec color = bvec::hexcolor(p->ammo[gun] == 0 || ammoratio >= 1.0f ? 0xFFFFFF : (ammoratio >= 0.5f ? 0xFFC040 : 0xFF0000));
        draw_text(label, textdrawpos.x, textdrawpos.y, color.r, color.g, color.b, alpha);

        pophudmatrix();
    }

    static inline bool ammobargunvisible(const fpsent *d, int gun, bool hideempty)
    {
        if(d->ammo[gun] > 0 || d->gunselect == gun) return true;
        if(hideempty) return false;
        if(m_efficiency) return gun!=GUN_PISTOL;
        if(m_insta) return gun==GUN_RIFLE;
        return true;
    }

    void drawammobar(int w, int h, fpsent *p)
    {
        if(m_insta) return;

        int NUMPLAYERGUNS = GUN_PISTOL - GUN_SG + 1;
        int numvisibleguns = NUMPLAYERGUNS;
        loopi(NUMPLAYERGUNS) if(!ammobargunvisible(p, GUN_SG + i, ammobarhideempty!=0)) numvisibleguns--;

        vec2 origin = vec2(ammobarx, ammobary).mul(vec2(w, h).div(ammobarscale));
        vec2 offsetdir = ammobarhorizontal ? vec2(1, 0) : vec2(0, 1);
        float stepsize = HICON_SIZE + ammobarsep;
        float initialoffset = (ammobaralign - 1) * (numvisibleguns - 1) * stepsize / 2;

        pushhudmatrix();
        hudmatrix.scale(ammobarscale, ammobarscale, 1);
        flushhudmatrix();

        int numskippedguns = 0;
        loopi(NUMPLAYERGUNS) if(ammobargunvisible(p, GUN_SG + i, ammobarhideempty!=0))
        {
            float offset = initialoffset + (i - numskippedguns) * stepsize;
            vec2 drawpos = vec2(offsetdir).mul(offset).add(origin);
            drawammobarcounter(drawpos, p, GUN_SG + i);
        }
        else numskippedguns++;

        pophudmatrix();
    }

    void gameplayhud(int w, int h)
    {
        pushhudmatrix();
        hudmatrix.scale(h/1800.0f, h/1800.0f, 1);
        flushhudmatrix();

        if(player1->state==CS_SPECTATOR)
        {
            int pw, ph, tw, th, fw, fh;
            text_bounds("  ", pw, ph);
            text_bounds("SPECTATOR", tw, th);
            th = max(th, ph);
            fpsent *f = followingplayer();
            text_bounds(f ? colorname(f) : " ", fw, fh);
            fh = max(fh, ph);
            draw_text("SPECTATOR", w*1800/h - tw - pw, 1650 - th - fh);
            if(f)
            {
                int color = statuscolor(f, 0xFFFFFF);
                draw_text(colorname(f), w*1800/h - fw - pw, 1650 - fh, (color>>16)&0xFF, (color>>8)&0xFF, color&0xFF);
            }
        }

        fpsent *d = hudplayer();
        if(d->state!=CS_EDITING)
        {
            if(d->state!=CS_SPECTATOR) drawhudicons(d);
            if(cmode) cmode->drawhud(d, w, h);
        }

        pophudmatrix();

        if(d->state!=CS_EDITING && d->state!=CS_SPECTATOR && d->state!=CS_DEAD)
        {
            if(ammobar) drawammobar(w, h, d);
        }

        if(!m_edit && !m_sp)
        {
            if(gameclock) drawgameclock(w, h);
            if(hudscore) drawhudscore(w, h);
        }
    }

    //NEW

    MODVARP(showtimeleft, 0, 1, 1);
    MODVARP(showpingdisplay, 0, 1, 1);
    MODVARP(shownetworkdisplay, 0, 0, 1);
    MODVARP(showstatsdisplay, 0, 1, 1);
    MODVARP(scaletimeleft, 0, 1, 1);

    static int displays = 0; //NEW
    inline void increasedisplaycount() { displays++; }
    inline int getdisplaycount() { return displays; }
    struct resetdisplaycount { resetdisplaycount(int) {} ~resetdisplaycount() { displays = 0; } };

    void renderping(int w, int h, int fonth)
    {
        if(!showpingdisplay || showfpsrange || (!isconnected(false, false) && !game::demoplayback)) return;
        fpsent *d = hudplayer();
        if(game::demoplayback && d == player1) return;
        if(d->aitype!=AI_NONE)
        {
            d = getclient(d->ownernum);
            if(!d) return;
        }
        float ping = d->highresping ? d->highresping : (float)d->ping;
        draw_textf(d->highresping ? "%.2f ms" : "%.0f ms", w-10*fonth, h-fonth*3/2, ping);
        increasedisplaycount();
    }

    void rendertimeleft(int w, int h, int fonth)
    {
        if(showtimeleft && !m_sp && hudplayer()->state!=CS_EDITING)
        {
            int t = m_edit ? lastmillis/1000 : (intermission ? 0 : max(maplimit-lastmillis, 0)/1000)*100/(scaletimeleft ? gamespeed : 100);
            int m = t/60, s = t%60;
            if(!(!m_edit && !m && s && s<30 && lastmillis%1000>=500))
                draw_textf("%s%d:%-2.2d", w-(10+(getdisplaycount()*4))*fonth, h-fonth*3/2, (!m_edit && !m) ? "\f3" : "", m, s);
            increasedisplaycount();
        }
    }

    void rendernetwork(int w, int h, int screenw, int screenh, int fonth)
    {
        resetdisplaycount rdc(0);
        if(!shownetworkdisplay || !curpeer || !curpeer->bandwidthStats.data) return;
        if(getdisplaycount() && screenw/float(screenh)<1.6f)
        {
            static int c = 0;
            if(c++ % 1000 == 0)
            {
                conoutf("screen resolution too low for network "
                        "display (turn pingdisplay and timeleft off)");
            }
            return;
        }
        clientbwstats *bw = (clientbwstats *)curpeer->bandwidthStats.data;
        defformatstring(bwtext, "%.2f kb/s (%d p/s) // %.2f kb/s (%d p/s)",
            bw->curincomingkbs, bw->curincomingpacketssec, bw->curoutgoingkbs, bw->curoutgoingpacketssec);
        int lengthoffset = strlen(bwtext)-39;
        if(lengthoffset>1) lengthoffset /= 2;
        else lengthoffset = 0;
        draw_text(bwtext, w-(22+(getdisplaycount()*5)+lengthoffset)*fonth, h-fonth*3/2);
    }

    static const char *sdfmtdefault = "Frags: \f0%f \f7Deaths: \f3%d \f7KpD: \f0%k \f7Acc: \f1%a%%"
                                      "[ctf] \fs\f7Flags: \f0%x\fr [ctf][collect] \fs\f7Skulls: \f0%x\fr[collect]";

    MODSVARFP(statsdisplayfmt, sdfmtdefault, if(!statsdisplayfmt[0]) setsvar("statsdisplayfmt", sdfmtdefault, false));

    bool renderstatsdisplay(int conw, int conh, int FONTH, int woffset, int roffset)
    {
        if(!showstatsdisplay) return false;
        string stats; *stats = 0;
        gamemod::getgamestate(hudplayer(), statsdisplayfmt, stats, sizeof(stats));
        int tw = text_width(stats);
        draw_text(stats, conw-max(5*FONTH, 2*FONTH+tw)-woffset, conh-FONTH*3/2-roffset);
        return true;
    }

    //NEW END

    int clipconsole(int w, int h)
    {
        if(cmode) return cmode->clipconsole(w, h);
        return 0;
    }

    VARP(teamcrosshair, 0, 1, 1);
    VARP(hitcrosshair, 0, 425, 1000);

    const char *defaultcrosshair(int index)
    {
        switch(index)
        {
            case 2: return "data/hit.png";
            case 1: return "data/teammate.png";
            default: return "data/crosshair.png";
        }
    }

    int selectcrosshair(vec &color)
    {
        fpsent *d = hudplayer();
        if(d->state==CS_SPECTATOR || d->state==CS_DEAD) return -1;

        if(d->state!=CS_ALIVE) return 0;

        int crosshair = 0;
        if(lasthit && lastmillis - lasthit < hitcrosshair) crosshair = 2;
        else if(teamcrosshair)
        {
            dynent *o = intersectclosest(d->o, worldpos, d);
            if(o && o->type==ENT_PLAYER && isteam(((fpsent *)o)->team, d->team))
            {
                crosshair = 1;
                color = vec(0, 0, 1);
            }
        }

        if(crosshair!=1 && !editmode && !m_insta)
        {
            if(d->health<=25) color = vec(1, 0, 0);
            else if(d->health<=50) color = vec(1, 0.5f, 0);
        }
        if(d->gunwait) color.mul(0.5f);
        return crosshair;
    }

    void lighteffects(dynent *e, vec &color, vec &dir)
    {
#if 0
        fpsent *d = (fpsent *)e;
        if(d->state!=CS_DEAD && d->quadmillis)
        {
            float t = 0.5f + 0.5f*sinf(2*M_PI*lastmillis/1000.0f);
            color.y = color.y*(1-t) + t;
        }
#endif
    }

    int maxsoundradius(int n)
    {
        switch(n)
        {
            case S_JUMP:
            case S_LAND:
            case S_WEAPLOAD:
            case S_ITEMAMMO:
            case S_ITEMHEALTH:
            case S_ITEMARMOUR:
            case S_ITEMPUP:
            case S_ITEMSPAWN:
            case S_NOAMMO:
            case S_PUPOUT:
                return 340;
            default:
                return 500;
        }
    }

    bool serverinfostartcolumn(g3d_gui *g, int i)
    {
        static const char * const names[] =  { "ping ", "players ", "mode ", "map ", "time ", "master ", "host ", "port ", "description ", "country" }; //NEW "country"
        static const float struts[] =        { 7,       7,          12.5f,   14,      7,      8,         14,      7,       24.5f,          3 };         //NEW country column
        extern int showcountry; //NEW
        if(size_t(i) >= (sizeof(names)/sizeof(names[0])-!showcountry)) return false; //NEW -!showcountry
        g->pushlist();
        g->text(names[i], 0xFFFF80, !i ? " " : NULL);
        if(struts[i]) g->strut(struts[i]);
        g->mergehits(true);
        return true;
    }

    void serverinfoendcolumn(g3d_gui *g, int i)
    {
        g->mergehits(false);
        g->column(i);
        g->poplist();
    }

    const char *mastermodecolor(int n, const char *unknown)
    {
        return (n>=MM_START && size_t(n-MM_START)<sizeof(mastermodecolors)/sizeof(mastermodecolors[0])) ? mastermodecolors[n-MM_START] : unknown;
    }

    const char *mastermodeicon(int n, const char *unknown)
    {
        return (n>=MM_START && size_t(n-MM_START)<sizeof(mastermodeicons)/sizeof(mastermodeicons[0])) ? mastermodeicons[n-MM_START] : unknown;
    }

    //NEW
    MODSVARFP(filterservercountry, "", for(char *c = filterservercountry; *c; c++) *c = toupper(*c));
    MODSVARFP(filterserverdesc, "", for(char *c = filterserverdesc; *c; c++) *c = tolower(*c));
    MODVARP(filterservermode, STARTGAMEMODE-1, STARTGAMEMODE-1, NUMGAMEMODES);
    MODSVARFP(filterservermodestring, "", for(char *c = filterservermodestring; *c; c++) *c = tolower(*c));
    MODVARP(filterservermod, INT_MIN, 0, INT_MAX);
    MODSVARFP(filterserverhost, "", for(char *c = filterserverhost; *c; c++) *c = toupper(*c));
    MODVARP(filterservermastermode, MM_START-1, MM_START-1, int(sizeofarray(mastermodenames))+MM_START);
    //NEW END

    bool serverinfoentry(g3d_gui *g, bool &shown, int i, const char *name, int port, const char *sdesc, const char *map, int ping, const vector<int> &attr, int np, int servermod, const char *country, const char *countrycode, int speccount) //NEW shown, servermod, country, countrycode, speccount
    {
        //NEW
        shown = false;
        if(filterservercountry[0] && (!countrycode || strcmp(filterservercountry, countrycode))) return false;
        if(filterservermod && filterservermod != servermod) return false;
        if(filterserverdesc[0])
        {
            string tmp; mod::strtool desc(tmp, sizeof(tmp));
            desc = sdesc; desc.lowerstring();
            if(!desc.find(filterserverdesc)) return false;
        }
        if(filterservermode != STARTGAMEMODE-1 && (attr.length()<2 || filterservermode != attr[1])) return false;
        else if(filterservermodestring[0])
        {
            if(attr.length()<2) return false;
            if(!strstr(server::modename(attr[1], ""), filterservermodestring)) return false;
        }
        if(filterserverhost[0])
        {
            string tmp; mod::strtool stname(tmp, sizeof(tmp));
            stname = name; stname.upperstring();
            if(!stname.find(filterserverhost)) return false;
        }
        if(filterservermastermode>-1 && (attr.length()<5 || filterservermastermode != attr[4])) return false;
        shown = true;
        //NEW END

        if(ping < 0 || attr.empty() || attr[0]!=PROTOCOL_VERSION)
        {
            switch(i)
            {
                case 0:
                    if(g->button(" ", 0xFFFFDD, "serverunk")&G3D_UP) return true;
                    break;

                case 1:
                case 2:
                case 3:
                case 4:
                case 5:
                    if(g->button(" ", 0xFFFFDD)&G3D_UP) return true;
                    break;

                case 6:
                    if(g->buttonf("%s ", 0xFFFFDD, NULL, name)&G3D_UP) return true;
                    break;

                case 7:
                    if(g->buttonf("%d ", 0xFFFFDD, NULL, port)&G3D_UP) return true;
                    break;

                case 8:
                    if(ping < 0)
                    {
                        if(g->button(sdesc, 0xFFFFDD)&G3D_UP) return true;
                    }
                    else if(g->buttonf("[%s protocol] ", 0xFFFFDD, NULL, attr.empty() ? "unknown" : (attr[0] < PROTOCOL_VERSION ? "older" : "newer"))&G3D_UP) return true;
                    break;

                //NEW
                case 9:
                    extern int showcountry; bool unused;
                    rendercountry(*g, countrycode, country, showcountry, &unused);
                    break;
                //NEW END
            }
            return false;
        }

        switch(i)
        {
            case 0:
            {
                const char *icon = attr.inrange(3) && np >= attr[3] ? "serverfull" : (attr.inrange(4) ? mastermodeicon(attr[4], "serverunk") : "serverunk");
                if(g->buttonf("%d ", 0xFFFFDD, icon, (int)ping)&G3D_UP) return true;
                break;
            }

            case 1:
                if(attr.length()>=4)
                {
                    //NEW
                    const char *a, *b;
                    if(speccount > 0)
                    {
                        a = "\f3%d/%d (%ds) ";
                        b = "%d/%d (%ds) ";
                    }
                    else
                    {
                        a = "\f3%d/%d ";
                        b = "%d/%d ";
                    }
                    if(g->buttonf(np >= attr[3] ? a : b, 0xFFFFDD, NULL, np, attr[3], speccount)&G3D_UP) return true;
                    //NEW END
                    //if(g->buttonf(np >= attr[3] ? "\f3%d/%d " : "%d/%d ", 0xFFFFDD, NULL, np, attr[3])&G3D_UP) return true; //NEW commented out
                }
                else if(g->buttonf("%d ", 0xFFFFDD, NULL, np)&G3D_UP) return true;
                break;

            case 2:
                if(g->buttonf("%s ", 0xFFFFDD, NULL, attr.length()>=2 ? server::modename(attr[1], "") : "")&G3D_UP) return true;
                break;

            case 3:
                if(g->buttonf("%.25s ", 0xFFFFDD, NULL, map)&G3D_UP) return true;
                break;

            case 4:
                if(attr.length()>=3 && attr[2] > 0)
                {
                    int secs = clamp(attr[2], 0, 59*60+59),
                        mins = secs/60;
                    secs %= 60;
                    if(g->buttonf("%d:%02d ", 0xFFFFDD, NULL, mins, secs)&G3D_UP) return true;
                }
                else if(g->buttonf(" ", 0xFFFFDD)&G3D_UP) return true;
                break;
            case 5:
                if(g->buttonf("%s%s ", 0xFFFFDD, NULL, attr.length()>=5 ? mastermodecolor(attr[4], "") : "", attr.length()>=5 ? server::mastermodename(attr[4], "") : "")&G3D_UP) return true;
                break;

            case 6:
                if(g->buttonf("%s ", 0xFFFFDD, NULL, name)&G3D_UP) return true;
                break;

            case 7:
                if(g->buttonf("%d ", 0xFFFFDD, NULL, port)&G3D_UP) return true;
                break;

            case 8:
                if(g->buttonf("%.25s", 0xFFFFDD, NULL, sdesc)&G3D_UP) return true;
                break;

            //NEW
            case 9:
                extern int showcountry; bool unused;
                rendercountry(*g, countrycode, country, showcountry, &unused);
                break;
            //NEW END
        }
        return false;
    }

    // any data written into this vector will get saved with the map data. Must take care to do own versioning, and endianess if applicable. Will not get called when loading maps from other games, so provide defaults.
    void writegamedata(vector<char> &extras) {}
    void readgamedata(vector<char> &extras) {}

    const char *savedconfig() { return "config.cfg"; }
    const char *restoreconfig() { return "restore.cfg"; }
    const char *defaultconfig() { return "data/defaults.cfg"; }
    const char *autoexec() { return "autoexec.cfg"; }
    const char *wcautoexec() { return "wcautoexec.cfg"; } //NEW
    const char *wcconfig() { return "wc-ng.cfg"; } //NEW
    const char *savedservers() { return "servers.cfg"; }
    const char *history() { return "history"; }

    void loadconfigs()
    {
        execident("playsong");

        execfile("auth.cfg", false);
    }
}
