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

// included in fpsgame/client.cpp

#include <sys/stat.h>

static void loopmodes(ident *id, uint *body)
{
    if (id->type != ID_ALIAS) return;
    identstack stack;
    loopi(NUMGAMEMODES)
    {
        tagval t;
        t.setstr(newstring(gamemodes[i].name));
        pusharg(*id, t, stack);
        id->flags &= ~IDF_UNKNOWN;
        execute(body);
    }
    poparg(*id);
}
COMMAND(loopmodes, "re");

static void filetime(const char *file)
{
    struct stat ft;
    if (stat(file, &ft) < 0) intret(0);
    intret((int)ft.st_ctime);
}
COMMAND(filetime, "s");

struct file
{
    char *cfile;
    struct stat stat;
};

static int comparefilescreated(const file a, const file b)
{
    if (a.stat.st_ctime > b.stat.st_ctime) return -1;
    if (a.stat.st_ctime < b.stat.st_ctime) return 1;
    else return 0;
}

static int comparefilesmodified(const file a, const file b)
{
    if (a.stat.st_mtime > b.stat.st_mtime) return -1;
    if (a.stat.st_mtime < b.stat.st_mtime) return 1;
    else return 0;
}

static int comparefilessize(const file a, const file b)
{
    if (a.stat.st_size > b.stat.st_size) return -1;
    if (a.stat.st_size < b.stat.st_size) return 1;
    else return 0;
}

static void loopfiles2(const char *folder, const char *ext, int max,
    const char *sorter, ident *id, uint *body)
{
    if (id->type != ID_ALIAS) return;

    identstack stack;

    vector<char *> files;
    listfiles(folder, ext, files);

    if (!files.length())
    {
        intret(0);
        return;
    }

    vector<file> files2;

    loopv(files)
    {
        file &f = files2.add();
        f.cfile = files[i];

        defformatstring(tmp)("%s%s.%s", folder, f.cfile, ext);
        path(tmp);

        if (stat(tmp, &f.stat) != 0)
            files2.pop();
    }

    files.shrink(0);

    if (!strcasecmp(sorter, "sort_created")) files2.sort(comparefilescreated);
    else if (!strcasecmp(sorter, "sort_modified")) files2.sort(comparefilesmodified);
    else if (!strcasecmp(sorter, "sort_size")) files2.sort(comparefilessize);
    else { conoutf(CON_ERROR, "unknown sorter method passed to loopfiles2!"); return; }

    max = clamp(max, 0, files2.length());

    loopi(max)
    {
        tagval t;
        t.setstr(files2[i].cfile);
        pusharg(*id, t, stack);
        id->flags &= ~IDF_UNKNOWN;
        execute(body);
    }

    if (files2.length())
        poparg(*id);

    files2.setsize(0);
}
COMMAND(loopfiles2, "ssisre");

int getactioncn(bool cubescript)
{
    vector<int> cns;

    if (m_ctf)
    {
        // add payers which currently have the flag

        loopv(ctfmode.flags)
        {
            // add players where pickup of flag happened <= 2.5 seconds ago

            ctfclientmode::flag &f = ctfmode.flags[i];
            fpsent *d = f.owner;
            if (!d || !*d->name) continue;

            if (lastmillis-f.owntime < 2500)
                cns.add(d->clientnum);
        }

        if (!cns.length())
        {
            loopv(ctfmode.flags)
            {
                // no cns yet, add players which currently have the flag

                ctfclientmode::flag &f = ctfmode.flags[i];
                fpsent *d = f.owner;

                if (!d || !*d->name)
                    continue;

                if (lastmillis-f.owntime < 2500)
                    cns.add(d->clientnum);
            }
        }
    }

    if (m_capture)
    {
        // add players which are in the near of the base and in the capture radius

        loopv(capturemode.bases)
        {
            captureclientmode::baseinfo &b = capturemode.bases[i];
            loopv(clients)
            {
                fpsent *d = clients[i];

                if (!d || !*d->name || d->state != CS_ALIVE)
                    continue;

                if (d->o.dist(b.o) >= (float)captureclientmode::CAPTURERADIUS)
                    continue;

                cns.add(d->clientnum);
            }
        }
    }

    if (!cns.length())
    {
        // add players which are currently attacking

        loopv(clients)
        {
            fpsent *d = clients[i];

            if (!d || !*d->name || d->state != CS_ALIVE)
                continue;

            if (d->attacking)
                cns.add(d->clientnum);
        }
    }

    if (!cns.length())
    {
        // we still have no cns, so add everyone, who isn't in spec and not dead

        loopv(clients)
        {
            fpsent *d = clients[i];

            if (!d || !*d->name || d->state != CS_ALIVE)
                continue;

            cns.add(d->clientnum);
        }
    }

    if (!cns.length())
    {
        if (cubescript)
            intret(-1);

        return -1;
    }

    int cn = cns.length() > 1 ? cns[rnd(cns.length())] : cns[0];

    if (cubescript)
        intret(cn);

    return cn;
}

ICOMMAND(getactioncn, "", (), getactioncn(true));

void getclientcountry(int *cn)
{
    fpsent *d = game::getclient(*cn);
    result(d && *d->country ? d->country : "??");
}
COMMAND(getclientcountry, "i");

void getclientcountrycode(int *cn)
{
    fpsent *d = game::getclient(*cn);
    result(d && *d->countrycode ? d->countrycode : "??");
}
COMMAND(getclientcountrycode, "i");

void clientcountry(int *cn)
{
    fpsent *d = game::getclient(*cn);
    conoutf("%s", d && *d->country ? d->country : "??");
}
COMMAND(clientcountry, "i");

void clientcountrycode(int *cn)
{
    fpsent *d = game::getclient(*cn);
    conoutf("%s", d && *d->countrycode ? d->countrycode : "??");
}
COMMAND(clientcountrycode, "i");

ICOMMAND(gettotalmillis, "", (), intret(totalmillis));
ICOMMAND(getlastmillis, "", (), intret(lastmillis));

void getplayercount()
{
    int count = 0;
    loopv(clients) if (clients[i]) count++;
    intret(++count);
}
COMMAND(getplayercount, "");

void getserverdescription()
{
    string hostname;
    const char *res = "- No Description -";
    if ((demoplayback && demohasservertitle) ||
        (curpeer && enet_address_get_host_ip(&curpeer->address, hostname, sizeof(hostname)) >= 0))
    {
        if (*servinfo)
            res = servinfo;
        else if(curpeer)
        {
            formatstring(hostname)("%s:%d", hostname, curpeer->address.port);
            res = hostname;
        }
        else
            res = "";
    }
    string desc;
    filtertext(desc, res);
    result(desc);
}
COMMAND(getserverdescription, "");

void gettimestamp()
{
    intret(int(time(NULL)));
}
COMMAND(gettimestamp, "");


void getgametimestamp()
{
    if (!game::gametimestamp)
    {
        intret(0);
        return;
    }
    time_t ts = gametimestamp+((lastmillis-mapstart)/1000);
    intret(int(ts));
}
COMMAND(getgametimestamp, "");


void formattimestamp(int *timestamp, const char *fmt)
{
    time_t ts = *timestamp;

    if (ts < 0)
    {
        mod::erroroutf_r("invalid timestamp passed to formattimestamp");
        return;
    }

    for(const char *s = fmt; *s; s++)
    {
        if (*s != '%')
            continue;

        switch (*++s)
        {
        case 'a': case 'A': case 'b': case 'B':
        case 'c': case 'd': case 'H': case 'I':
        case 'j': case 'm': case 'M': case 'p':
        case 'r': case 'S': case 't': case 'U':
        case 'w': case 'W': case 'x': case 'X':
        case 'y': case 'Y': case 'z': case 'Z':
        case '%': continue;

        default:
            mod::erroroutf_r("invalid format specifier '%c' passed to formattimestamp", *s);
            return;
        }
    }

    struct tm *tm = localtime(&ts);
    string buf;
    strftime(buf, sizeof(buf), fmt, tm);
    result(buf);
}
COMMAND(formattimestamp, "is");

ICOMMAND(getosstring, "", (), result(mod::getosstring()));
