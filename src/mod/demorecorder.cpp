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

namespace game
{
    void demorecorder_initflags(ucharbuf &p);
    void demorecorder_initbases(ucharbuf &p);
}

using namespace game;

namespace mod {
namespace demorecorder
{
    static stream *f = NULL;
    bool demorecord = false;
    char *demofile = NULL;
    int demostart = 0;

    bool parsingmapchangepacket = false;
    int servmsgcmdignoremillis = 0;

    uint demoid = 0;
    vector<uint> timerids;
    llong rawdemosize = 0;

    MODVARP(clientdemorecordextinfo, 0, 1, 1);
    MODVARP(clientdemoskipservmsg, 0, 0, 1);
    MODVARP(clientdemoskipservcmds, 0, 1, 1);
    MODVARP(clientdemoautorecord, 0, 0, 1);
    MODVARP(clientdemocompresslevel, Z_NO_COMPRESSION, 5, Z_BEST_COMPRESSION);
    ICOMMAND(clientdemorecording, "", (), intret(demorecord ? 1 : 0));
    ICOMMAND(clientdemorawsize, "", (), intret((int)rawdemosize));

    const char *genproperdemofilename(char *buf, size_t len)
    {
        strtool file(buf, len);
        string tmp;

        time_t t = time(NULL);
        strftime(tmp, MAXSTRLEN, "%Y.%m.%d_%H_%M_%S_", localtime(&t));
        file = tmp;

        filtertext(tmp, servinfo);
        fixdemofilestring(tmp);

        {
            const char *p = *tmp ? tmp : "no_description";
            file.append(p, min(strlen(p), size_t(75)));
        }

        ENetAddress address;
        address.host = curpeer ? curpeer->address.host : 0;
        address.port = curpeer ? curpeer->address.port : 0;

        if (enet_address_get_host_ip(&address, tmp, MAXSTRLEN) >= 0)
            file.fmt("_%s_%u_", tmp, address.port);
        else
            file += "??";

        const char *gamemode = m_valid(game::gamemode) ? gamemodes[game::gamemode-STARTGAMEMODE].name : NULL;

        if (gamemode)
        {
            copystring(tmp, gamemode);

            string map;
            filtertext(map, getclientmap());

            fixdemofilestring(tmp);
            fixdemofilestring(map);

            file << tmp << '_' << map;
        }

        return file.getbuf();
    }

    void fixdemofilestring(char *str)
    {
        for(char *p = str; *p; p++)
        {
            switch (*p)
            {
                case '.':
                    if (p[1] == '.')
                    {
                        p[0] = '_'; p[1] = '_';
                        p++;
                    }
                    break;
                case '(': case ')': case '[': case ']':
                case '^': case '|': case '\\': case '"':
                    *p = '_';
                    break;
                default:
                    if (!((*p >= '@' && *p <= '}') || (*p >= 'A' && *p <= '_') ||
                          (*p >= '0' && *p <= '9') || (*p >= '!' && *p <= '.')))
                    {
                       *p = '_';
                    }
            }
        }
    }

    void writedemo(int chan, void *data, int len, bool nostamp, int offsetlen)
    {
        if (!demorecord || len <= 0 || (len > MAXTRANS && chan != 2))
            return;

        if (!nostamp)
        {
            int stamp[3] = { totalmillis - demostart, chan, len };
            lilswap(stamp, 3);
            f->write(stamp, sizeof(stamp));
        }

        len = len-offsetlen;
        rawdemosize += len;

        f->write(data, len);
    }

    static void putdemoinfomsg(void *cbdata, uint timerid)
    {
        uint &id = *(uint*)cbdata;

        uchar buf[MAXTRANS];
        ucharbuf p(buf, sizeof(buf));

        if (!demorecord || demoid != id)
            goto error;

        string servinfotmp;
        copystring(servinfotmp, servinfo);
        filtertext(servinfotmp, servinfo);

        string serverip;

        ENetAddress address;
        address.host = curpeer ? curpeer->address.host : 0;
        if (enet_address_get_host_ip(&address, serverip, MAXSTRLEN) < 0)
            copystring(serverip, "??");

        putint(p, N_SERVMSG);

        defformatstring(info)("demo recorded on %s (%s:%u)",
                              *servinfotmp ? servinfotmp : "<no description>", serverip,
                              curpeer ? curpeer->address.port : 0);

        sendstring(info, p);

        writedemo(1, p.buf, p.length());

        error:;
        delete &id;
        timerids.removeobj(timerid);
    }

    static void welcomepacket(ucharbuf &p)
    {
        putint(p, N_WELCOME);

        if (!parsingmapchangepacket)
        {
            putint(p, N_MAPCHANGE);
            sendstring(getclientmap(), p);
            putint(p, gamemode);
            putint(p, 0); // notgotitems
        }

        putint(p, N_TIMEUP);
        putint(p, max(maplimit-lastmillis, 0)/1000);

        putint(p, N_CURRENTMASTER);
        putint(p, mastermode);

        clients.add(player1);

        loopv(clients)
        {
            fpsent *d = clients[i];

            if (!d || !*d->name || d->privilege <= PRIV_NONE)
                continue;

            putint(p, d->clientnum);
            putint(p, d->privilege);
        }

        putint(p, -1);

        if (gamepaused)
        {
            putint(p, N_PAUSEGAME);
            putint(p, 1);
        }

        putint(p, N_RESUME);

        loopv(clients)
        {
            fpsent *d = clients[i];

            if (!d || !*d->name)
                continue;

            putint(p, d->clientnum);
            putint(p, d->state);
            putint(p, d->frags);
            putint(p, d->flags);
            putint(p, d->quadmillis);

            putint(p, d->lifesequence);
            putint(p, d->health);
            putint(p, d->maxhealth);
            putint(p, d->armour);
            putint(p, d->armourtype);
            putint(p, d->gunselect);

            loopj(GUN_PISTOL-GUN_SG+1)
                putint(p, d->ammo[GUN_SG+j]);
        }

        putint(p, -1);

        loopv(clients)
        {
            fpsent *d = clients[i];

            if (!d || !*d->name)
                continue;

            if (d->aitype != AI_NONE)
            {
                putint(p, N_INITAI);
                putint(p, d->clientnum);
                putint(p, d->ownernum);
                putint(p, d->aitype);
                putint(p, d->skill);
                putint(p, d->playermodel);
                sendstring(d->name, p);
                sendstring(d->team, p);
            }
            else
            {
                putint(p, N_INITCLIENT);
                putint(p, d->clientnum);
                sendstring(d->name, p);
                sendstring(d->team, p);
                putint(p, d->playermodel);
            }
        }

        clients.pop();

        if (m_ctf)
            demorecorder_initflags(p);

        if (m_capture)
            demorecorder_initbases(p);
    }

    void setupdemorecord(const char *name, bool force)
    {
        if (!force)
        {
            if (!m_mp(gamemode))
                return;

            if (!isconnected(false, false))
            {
                erroroutf_r("you need to connect to a server first.");
                return;
            }
        }

        if (demorecord)
        {
            if (!force)
            {
                erroroutf_r("already recording client side demo, type '/stopclientdemo' to stop demo recording");
                return;
            }
            else
                stopdemorecord();
        }

        string tmp1, tmp2;
        strtool dir(tmp1, sizeof(tmp1));
        strtool rdir(tmp2, sizeof(tmp2));

        dir = gethomedir();
        if (!dir.empty()) dir += PATHDIV;
        dir += CLIENT_DEMO_DIRECTORY;
        dir.fixpathdiv();

        if (!fileexists(dir.str(), "r") && !createdir(dir.str()))
        {
            conoutf(CON_ERROR, "failed to create demo directory (%s)", CLIENT_DEMO_DIRECTORY);
            return;
        }

        string demofilename;
        string demomode;
        copystring(demomode, m_valid(game::gamemode) ? gamemodes[game::gamemode-STARTGAMEMODE].name : "unknown");
        demorecorder::fixdemofilestring(demomode);

        dir += PATHDIV;
        dir += demomode;

        if (!fileexists(dir.str(), "r") && !createdir(dir.str()))
        {
            conoutf(CON_ERROR, "failed to create demo directory (%s)", dir.str());
            return;
        }

#ifdef WIN32
        static const char *fmt = "%s\\%s.dmo";
#else
        static const char *fmt = "%s/%s.dmo";
#endif

        rdir = CLIENT_DEMO_DIRECTORY;
        rdir += PATHDIV;
        rdir += demomode;

        defformatstring(dmo)(fmt, rdir.str(), name && *name ? name :
                             genproperdemofilename(demofilename, sizeof(demofilename)));

        DELETEA(demofile);

        demofile = newstring(dmo);
        f = opengzfile(demofile, "wb", NULL, clientdemocompresslevel);

        if (!f)
        {
            erroroutf_r("couldn't open (%s) for writing", demofile);
            DELETEA(demofile);
            return;
        }

        demorecord = true;
        demoid++;

        event::run(event::CLIENT_DEMO_START, "s", demofile);
        conoutf("\f3recording client demo ...");
        demostart = totalmillis;

        demoheader hdr;
        memcpy(hdr.magic, DEMO_MAGIC, sizeof(hdr.magic));
        hdr.version = DEMO_VERSION;
        hdr.protocol = PROTOCOL_VERSION;
        lilswap(&hdr.version, 2);
        f->write(&hdr, sizeof(demoheader));

        uchar buf[MAXTRANS];
        ucharbuf p(buf, sizeof(buf));
        welcomepacket(p);

        writedemo(1, p.buf, p.len);

        timerids.add(addtimerfunction(clientdemoautorecord ? 500 : 0, true, &putdemoinfomsg, new int(demoid)));

        // N_DEMORECORDER_DEMOINFO

        p.len = 0;
        self::putdemoinfoobj(p);

        // N_DEMORECORDER_DEMOINFO END
    }

    void stopdemorecord()
    {
        if (!demorecord)
            return;

        DELETEP(f);
        conoutf("wrote client demo %s", demofile);
        demorecord = false;
        demostart = 0;

        event::run(event::CLIENT_DEMO_END, "s", demofile);
        rawdemosize = 0;
    }

    namespace self
    {
        static void clientmsg(void *data, int len, fpsent *d = player1)
        {
            uchar buf[128];
            ucharbuf p(buf, sizeof(buf));

            putint(p, N_CLIENT);
            putint(p, d->clientnum);
            putint(p, len);

            writedemo(1, p.buf, p.length()+len, false, len);
            writedemo(-1, data, len, true);
        }

        void switchname(const char *newname)
        {
            uchar buf[64];
            ucharbuf p(buf, sizeof(buf));

            putint(p, N_SWITCHNAME);
            sendstring(newname, p);

            clientmsg(p.buf, p.length());
        }

        void text(const char *text)
        {
            string ftext;
            copystring(ftext, text);
            filtertext(ftext, text);

            if (clientdemoskipservcmds)
            {
                int i = 0;
                while (ftext[i] == ' ')
                    i++;

                for(const char *p = "@#"; *p; p++)
                {
                    if (*p == ftext[i])
                    {
                        servmsgcmdignoremillis = totalmillis+(player1->ping*3);
                        return;
                    }
                }
            }

            uchar buf[MAXTRANS];
            ucharbuf p(buf, sizeof(buf));
            putint(p, N_TEXT);
            sendstring(ftext, p);

            clientmsg(p.buf, p.length());
        }

        void gunselect(int gun)
        {
            uchar buf[64];
            ucharbuf p(buf, sizeof(buf));

            putint(p, N_GUNSELECT);
            putint(p, gun);

            clientmsg(p.buf, p.length());
        }

        void shoteffect(void *d, int gun, int id, const vec &from, const vec &to)
        {
            uchar buf[64];
            ucharbuf p(buf, sizeof(buf));

            putint(p, N_SHOTFX);
            putint(p, ((fpsent *)d)->clientnum);
            putint(p, gun);
            putint(p, id);

            loopi(3)
                putint(p, int(from[i]*DMF));

            loopi(3)
                putint(p, int(to[i]*DMF));

            clientmsg(p.buf, p.length(), (fpsent*)d);
        }

        void sound(void *d, int n)
        {
            uchar buf[64];
            ucharbuf p(buf, sizeof(buf));

            putint(p, N_SOUND);
            putint(p, n);
            clientmsg(p.buf, p.length(), (fpsent*)d);
        }

        void ping(int n)
        {
            uchar buf[64];
            ucharbuf p(buf, sizeof(buf));

            putint(p, N_CLIENTPING);
            putint(p, n);
            clientmsg(p.buf, p.length());
        }

        void posupdate(void *d, void *data, int len)
        {
            writedemo(0, data, len);
        }

        void jumppad(int jp, void *d)
        {
            uchar buf[64];
            ucharbuf p(buf, sizeof(buf));

            putint(p, N_JUMPPAD);
            putint(p, ((fpsent*)d)->clientnum);
            putint(p, jp);
            writedemo(0, p.buf, p.length());
        }

        void teleport(int n, int td, void *d)
        {
            uchar buf[64];
            ucharbuf p(buf, sizeof(buf));

            putint(p, N_TELEPORT);
            putint(p, ((fpsent *)d)->clientnum);
            putint(p, n);
            putint(p, td);
            writedemo(0, p.buf, p.length());
        }

        void putextinfoobj(extinfo::player *ep)
        {
            uchar buf[sizeof(extinfo::player)*2];
            ucharbuf p(buf, sizeof(buf));

            putint(p, N_DEMORECORDER_EXTINFO_INT);
            putint(p, extinfo::player::VERSION);
            putint(p, sizeof(extinfo::player));

            if ((p.maxlen-p.length()) < (int)sizeof(extinfo::player))
                return;

            ep->swap();
            memcpy(p.buf+p.length(), ep, sizeof(extinfo::player));
            ep->swap();

            p.len += sizeof(extinfo::player);

            writedemo(2, p.buf, p.length());
        }

        extinfo::player *getextinfoobj(ucharbuf &p, extinfo::player *buf, bool getptr)
        {
            if (getint(p) != extinfo::player::VERSION || getint(p) != (int)sizeof(extinfo::player) ||
                p.maxlen-p.len != (int)sizeof(extinfo::player))
                 return NULL;

            if (!buf)
            {
                if (getptr)
                    buf = (extinfo::player*)(p.buf+p.len);
                else
                    buf = new extinfo::player(*(extinfo::player*)(p.buf+p.len));
            }

            p.len += sizeof(extinfo::player);

            buf->name[sizeof(buf->name)-1] = 0;
            buf->team[sizeof(buf->team)-1] = 0;

            buf->swap();

            return buf;
        }

        void putdemoinfoobj(ucharbuf &p)
        {
            putint(p, N_DEMORECORDER_DEMOINFO_INT);

            union { int i[2]; time_t t; } t;
            memset(&t, 0, sizeof(t));

            t.t = time(NULL);

            lilswap(t.i, sizeofarray(t.i));

            loopi(sizeofarray(t.i))
                putint(p, t.i[i]);

            putint(p, curpeer ? (int)curpeer->address.host : -1);
            putint(p, curpeer ? (int)curpeer->address.port : -1);
            sendstring(servinfo, p);
            putint(p, gamemode);
            sendstring(getclientmap(), p);
            putint(p, 0);

            writedemo(2, p.buf, p.length());
        }

        demoinfo_t *getdemoinfoobj(ucharbuf &p, demoinfo_t *demoinfo)
        {
            union { int i[2]; time_t t; } t;

            loopi(sizeofarray(t.i))
                t.i[i] = getint(p);

            lilswap(t.i, sizeofarray(t.i));

            demoinfo->time = t.t;

            demoinfo->host = (uint)getint(p);
            demoinfo->port = (uint)getint(p);

            getstring(demoinfo->servinfo, p, sizeof(demoinfo->servinfo));
            filtertext(demoinfo->servinfo, demoinfo->servinfo);

            demoinfo->mode = getint(p);

            getstring(demoinfo->map, p, sizeof(demoinfo->map));
            filtertext(demoinfo->map, demoinfo->map);

            demoinfo->ok = !p.overread();

            return demoinfo;
        }
    }

    void freetimerdata(void *data) { delete (uint*)data; }

    void startup() { }

    void shutdown()
    {
        stopdemorecord();

        loopv(timerids)
            removetimerfunction(timerids[i], freetimerdata);
    }

    ICOMMAND(stopclientdemo, "", (), stopdemorecord());
    ICOMMAND(recordclientdemo, "s", (const char *name), setupdemorecord(name));

    ICOMMAND(clientdemogetdemodir, "", (), { result(CLIENT_DEMO_DIRECTORY); });
}

using namespace demorecorder;

namespace searchdemo
{
    // #define IGNOREDEMOVERSION

    static atomic<int> debug;
    MODVARFP(searchdemodebug, 0, 0, 1, debug = searchdemodebug);

    static void consoleoutf(const char *fmt, const char *dbgfmt, ...)
    {
        bool dbg = debug!=0;
        char dbgfmtstr[1024];
        strtool msg;
        va_list v;
        va_start(v, dbgfmt);
        if (dbg)
        {
            strtool fmtstr(dbgfmtstr, sizeof(dbgfmtstr));
            fmtstr << fmt << "\f7(debug " << dbgfmt << ")";
        }
        msg.vfmt(dbg ? dbgfmtstr : fmt, v);
        va_end(v);
        addthreadmessage(msg);
    }

    #define searchdemodbgcode(p, c) {} p; if (searchdemodebug) do { c; } while(0)
    #define consoleout(msg, dbgmsg) consoleoutf(msg, dbgmsg)

    static bool jointhread = false;
    static SDL_Thread *searchdemothread = NULL;
    static SDL_mutex *searchmutex = NULL;

    static const int MAXTHREADS = 32;
    MODVARP(searchdemothreadlimit, 1, min(guessnumcpus(), MAXTHREADS), min(guessnumcpus(), MAXTHREADS));
    MODVARP(searchdemomaxresults, 0, 100, 250);
    MODVARP(searchdemomindemosize, 0, 0, 0x7FFFFFFF);
    ICOMMAND(searchdemoactive, "", (), intret(searchdemothread ? 1 : 0));

    template<class T>
    static inline bool readdemobuf(uint &position, const llong &filesize, void *buf, T **p)
    {
        if (position+sizeof(T) <= (ullong)filesize)
        {
            *p = (T*)(((char*)buf)+position);
            position += sizeof(**p);
            return true;
        }
        return false;
    }

    llong parsedemo(parsedemo_t *pd)
    {
        //
        // Search Demo for Extinfo Informations.
        // On sucess, this function returns the time it took to process the demo.
        // On error, this functions returns < 0.
        //

        #define RETURN_ERROR(e, errstr) \
        do \
        { \
            if (udata) free(udata); \
            if (filebuf) free(filebuf); \
            if (f) delete f; \
            if (pd->result) pd->result->deletecontents(); \
            if (pd->error) copystring(pd->error, errstr); \
            if (pd->packetcount) *pd->packetcount = packetnum; \
            return e; \
        } while(0)

        #define READ(p, e) do { if (!readdemobuf(position, ulen, udata, p)) e; } while(0)
        #define JUMP(s, c, e) do { if (position+(int)s <= ulen) { c; position += s; } else e; } while(0)

        static const int MAX_DEMO_SIZE = 16*1024*1024;
        static const int MAX_UNCOMPRESS_SIZE = MAX_DEMO_SIZE*12;

        llong ticks = getmicroseconds()%0x7FFFFFFFFFFFFFFFLL;

        int packetnum = 0; // packet counter

        uint position = 0;
        llong filesize = 0;
        int extinfocount = 0;

        stream *f = NULL;
        void *filebuf = NULL;

        size_t ulen;
        void *udata = NULL;

        {
            //
            // Uncompress the whole demo at once and work with a pointer,
            // because calling read() all the time is SLOW.
            //

            if (pd->uncompressticks)
                *pd->uncompressticks = getmicroseconds();

            lockstream();
            f = openfile(pd->demo, "rb");
            unlockstream();

            if (!f)
                RETURN_ERROR(ERROR_FAILED_TO_OPEN_DEMO_FILE, "failed to open demo file");

            filesize = f->size();

            if (filesize > MAX_DEMO_SIZE)
                RETURN_ERROR(ERROR_DEMO_FILE_TOO_BIG, "demo file too big");

            if (filesize < pd->mindemosize)
            {
                delete f;
                f = NULL;

                if (pd->demofilesize)
                    *pd->demofilesize = 0;

                goto returnoknoparseticks;
            }

            if (!filesize)
                RETURN_ERROR(ERROR_FAILED_TO_OPEN_DEMO_FILE, "corrupted demo file");

            filebuf = malloc(filesize);

            if (!filebuf)
            {
                defformatstring(errmsg)("failed to allocate %.2f mb, for reading file into memory", filesize/1024.0f/1024.0f);
                RETURN_ERROR(ERROR_INVALID_UNCOMPRESS_SIZE, errmsg);
            }

            if ((llong)f->read(filebuf, filesize) != filesize)
                RETURN_ERROR(ERROR_FAILED_TO_READ_DEMO_FILE, "failed to read demo file into buffer");

            switch (uncompress(filebuf, filesize, &udata, ulen, MAX_UNCOMPRESS_SIZE))
            {
                case UNCOMPRESS_OK: break;
                case UNCOMPRESS_ERROR_OUTPUT_SIZE_TOO_BIG:
                {
                    defformatstring(errmsg)("uncompress buffer size is too big (%.2f mb).", ulen/1024.0f/1024.0f);
                    if (llong(ulen/20) > filesize) concatstring(errmsg, " corrupted file?");
                    RETURN_ERROR(ERROR_INVALID_UNCOMPRESS_SIZE, errmsg);
                }
                case UNCOMPRESS_ERROR_FAILED_TO_ALLOCATE_BUFFER:
                {
                    defformatstring(errmsg)("failed to allocate %.2f mb, for uncompressing", ulen/1024.0f/1024.0f);
                    RETURN_ERROR(ERROR_INVALID_UNCOMPRESS_SIZE, errmsg);
                }
                case UNCOMPRESS_ERROR_INVALID_BUFFER:
                {
                    RETURN_ERROR(ERROR_FAILED_TO_OPEN_DEMO_FILE, "corrupted demo file");
                }
                case UNCOMPRESS_ERROR_ZLIB:
                {
                    RETURN_ERROR(ERROR_FAILED_TO_INIT_ZLIB, "failed to init zlib");
                }
                case UNCOMPRESS_ERROR_OTHER:
                {
                    RETURN_ERROR(ERROR_FAILED_TO_UNCOMPRESS_DEMO, "failed to uncompress demo file in memory (corrupted demo?)");
                }
                default: abort();
            }

            if (pd->stripdemo)
                pd->stripdemo->buf = udata;

            if (pd->demofilesize)
                *pd->demofilesize = ulen;

            free(filebuf);
            filebuf = NULL;

            delete f;
            f = NULL;

            //filesize = 0;

            if (pd->uncompressticks)
                *pd->uncompressticks = getmicroseconds()-*pd->uncompressticks;
        }

        if (pd->parsedemoticks)
            *pd->parsedemoticks = getmicroseconds();

        if (pd->demomillis)
            *pd->demomillis = 0;

        {
            demoheader *hdr;
            READ(&hdr, RETURN_ERROR(ERROR_FAILED_TO_READ_PACKET, "failed to read demo header"));

            if (memcmp(hdr->magic, DEMO_MAGIC, sizeof(hdr->magic)))
                RETURN_ERROR(ERROR_INVALID_DEMO_HEADER, "invalid demo header");

            lilswap(&hdr->version, 2);

            #ifndef IGNOREDEMOVERSION
            if (hdr->version != DEMO_VERSION || hdr->protocol != PROTOCOL_VERSION)
                RETURN_ERROR(ERROR_INVALID_DEMO_VERSION, "demo version");
            #endif

            if (pd->stripdemo)
                pd->stripdemo->write(hdr, sizeof(*hdr));
        }

        for (;; packetnum++)
        {
            int *chan, *len, *nextplayback;

            READ(&nextplayback,
            {
                if (packetnum >= 10)
                {
                    if (!pd->stripdemo && !pd->demomillis && !extinfocount)
                        RETURN_ERROR(ERROR_DEMO_CONTAINS_NO_EXTINFO, "demo contains no extinfo");

                    goto returnok;
                }
                else
                    RETURN_ERROR(ERROR_FAILED_TO_READ_PACKET, "read error (nextplayback 2)");
            });

            READ(&chan, RETURN_ERROR(ERROR_FAILED_TO_READ_PACKET, "read error (packet channel)"));
            READ(&len, RETURN_ERROR(ERROR_FAILED_TO_READ_PACKET, "read error (packet length)"));

            lilswap(chan, 1);
            lilswap(len, 1);
            lilswap(nextplayback, 1);

            if (uint(*len) > (*chan == 2 ? 10*1024*1024u : MAXTRANS))
                RETURN_ERROR(ERROR_INVALID_PACKET_SIZE, "invalid packet size");

            if (*chan != 2)
            {
                if (pd->stripdemo)
                    pd->stripdemo->position = position;

                JUMP(*len, {}, RETURN_ERROR(ERROR_FAILED_TO_JUMP_BUF, "failed to jump packet buffer"));

                if (pd->stripdemo)
                    pd->stripdemo->write(*len, *chan, *nextplayback);

                continue;
            }

            if (position+uint(*len) > ulen)
                RETURN_ERROR(ERROR_FAILED_TO_READ_PACKET, "failed to read extinfo packet");

            if (pd->stripdemo)
            {
                position += *len;

                pd->stripdemo->skippacketcount++;
                continue;
            }

            if (pd->demomillis)
            {
                position += *len;

                *pd->demomillis = *nextplayback;
                continue;
            }

            ucharbuf p((uchar*)udata+position, *len);
            position += *len;

            switch (int type = getint(p))
            {
                case N_SENDMAP:
                case N_SENDDEMO:
                    break;

                case N_DEMORECORDER_EXTINFO_SERVER:
                case N_DEMORECORDER_EXTINFO_EXT:
                case N_DEMORECORDER_EXTINFO_INT:
                {
                    extinfocount++;

                    // get a pointer to the extinfo object in the demo buffer
                    extinfo::player *ep = demorecorder::self::getextinfoobj(p, NULL, true);

                    if (!ep || !pd->fp || !pd->result)
                        break;

                    bool update = false;
                    bool ok = false;

                    const extinfo::player &fp = *pd->fp;
                    vector<extinfo::player*> &result = *pd->result;

                    if (*fp.name)          { ok = strstr(ep->name, fp.name) != NULL;  if (!ok) goto cont; }
                    if (fp.frags > -1)     { ok = ep->frags >= fp.frags;              if (!ok) goto cont; }
                    if (fp.deaths > -1)    { ok = ep->deaths >= fp.deaths;            if (!ok) goto cont; }
                    if (fp.flags > -1)     { ok = ep->flags >= fp.flags;              if (!ok) goto cont; }
                    if (fp.teamkills > -1) { ok = ep->teamkills >= fp.teamkills;      if (!ok) goto cont; }
                    if (fp.ping > -1)      { ok = ep->ping >= fp.ping;                if (!ok) goto cont; }
                    if (fp.acc > -1)       { ok = ep->acc >= fp.acc;                  if (!ok) goto cont; }
                    if (fp.priv > -1)      { ok = ep->priv >= fp.priv;                if (!ok) goto cont; }

                    ok = false;

                    loopvj(result)
                    {
                        if (result[j]->ip.ui32 == ep->ip.ui32)
                        {
                            if (*fp.name)
                                ok = !strcmp(result[j]->name, ep->name);

                            if (!ok)
                            {
                                if (result[j]->cn == ep->cn)
                                    ok = true;
                            }

                            if (!ok)
                                continue;

                            *result[j] = *ep;
                            update = true;

                            break;
                        }
                    }

                    if (update)
                        continue;

                    result.add(new extinfo::player(*ep));
                    continue;

                    cont:;
                    break;
                }

                case N_DEMORECORDER_DEMOINFO_SERVER:
                case N_DEMORECORDER_DEMOINFO_EXT:
                case N_DEMORECORDER_DEMOINFO_INT:
                {
                    if (!pd->demoinfo)
                        break;

                    demorecorder::self::getdemoinfoobj(p, pd->demoinfo);

                    if (!pd->demoinfo->ok || (pd->gamemode >= 0 && pd->demoinfo->mode != pd->gamemode))
                        goto returnok;

                    string buf;

                    if (pd->servername && *pd->servername)
                    {
                        strtool sname(buf, sizeof(buf));

                        sname = pd->demoinfo->servinfo;
                        sname.lowerstring();

                        if (!sname.find(pd->servername))
                            goto returnok;
                    }

                    if (pd->mapname && *pd->mapname)
                    {
                        strtool mname(buf, sizeof(buf));

                        mname = pd->demoinfo->map;
                        mname.lowerstring();

                        if (!mname.find(pd->mapname))
                            goto returnok;
                    }

                    break;
                }

                default:
                    conoutf_r(CON_WARN, "\fs\f3warning:\fr demo (%s) contains unknown packet type (%d)", pd->demo, type);
            }

        }

        returnoknoparseticks:;

        if (pd->parsedemoticks)
        {
            *pd->parsedemoticks = 0;
            goto freebuf;
        }

        returnok:;

        if (extinfocount && pd->gamemode > -1 && (!pd->demoinfo->ok || pd->demoinfo->mode != pd->gamemode))
        {
            conoutf_r(CON_WARN, "\fs\f3warning:\fr demo (%s) contains extinfo packets, but no demoinfo packet", pd->demo);

            if (pd->result)
                pd->result->deletecontents();
        }

        if (pd->parsedemoticks)
            *pd->parsedemoticks = getmicroseconds()-*pd->parsedemoticks;

        freebuf:;

        if (udata)
            free(udata);

        if (pd->packetcount)
            *pd->packetcount = packetnum;

        return (getmicroseconds()%0x7FFFFFFFFFFFFFFFLL)-ticks;
    }

    int checkdemothread(void *data);

    struct demothread_t
    {
        char *realdemofile;

        llong mindemosize;
        llong totaltime;
        int threadid;

        struct
        {
            string error;
            string demo;
            vector<extinfo::player*> result;
            llong demofilesize;
            ullong uncompressticks;
            ullong parsedemoticks;
            uint packetcount;
        } pdstorage;

        parsedemo_t pd;

        bool isprocessing;
        int processed;
        SDL_mutex *mutex;
        SDL_cond *cond;
        bool jointhread;
        bool processresult;
        bool waiting;
        SDL_Thread *thread;
        SDL_cond *availablecond;

        int filecount;

        void lock() { SDL_mutexP(mutex); }
        void unlock() { SDL_mutexV(mutex); }
        void available() { SDL_CondSignal(availablecond); }

        bool wait()
        {
            lock();
            waiting = true;
            isprocessing = false;

            if (jointhread)
            {
                waiting = false;
                unlock();
                return false;
            }

            available();
            SDL_CondWait(cond, mutex);
            waiting = false;
            return true;
        }

        void signal()
        {
            isprocessing = true;
            SDL_CondSignal(cond);
        }

        void join()
        {
            lock();
            jointhread = true;
            unlock();

            signal();
            SDL_WaitThread(thread, NULL);
        }

        bool iswaiting()
        {
            SDL_Mutex_Locker ml(mutex);
            return waiting;
        }

        bool isactive()
        {
            SDL_Mutex_Locker ml(mutex);
            return isprocessing;
        }

        bool needresultprocessing()
        {
            SDL_Mutex_Locker ml(mutex);
            return processresult;
        }

        int processcount()
        {
            SDL_Mutex_Locker ml(mutex);
            return processed;
        }

        void createthread()
        {
            thread = SDL_CreateThread(&checkdemothread, this);

            if (!thread)
                abort();
        }

        void reset()
        {
            pd.result->setsize(0);
            pd.demoinfo = NULL;
            *pd.packetcount = 0;
            *pd.demofilesize = 0;
            *pd.uncompressticks = 0;
            *pd.parsedemoticks = 0;
            isprocessing = false;
            processresult = false;
        }

        demothread_t(const extinfo::player &fp, int filecount, int gamemode, const char *servername,
                     const char *mapname, llong mindemosize, int threadid, SDL_cond *availablecond) :
                     threadid(threadid), mutex(SDL_CreateMutex()), cond(SDL_CreateCond()), jointhread(false),
                     waiting(false), availablecond(availablecond), filecount(filecount)
        {
            if (!mutex || !cond)
                abort();

            pd.fp = &fp;
            pd.gamemode = gamemode;
            pd.servername = servername;
            pd.mapname = mapname;
            pd.mindemosize = mindemosize;

            pd.error = pdstorage.error;
            pd.demo = pdstorage.demo;
            pd.result = &pdstorage.result;
            pd.packetcount = &pdstorage.packetcount;
            pd.demofilesize = &pdstorage.demofilesize;
            pd.uncompressticks = &pdstorage.uncompressticks;
            pd.parsedemoticks = &pdstorage.parsedemoticks;

            reset();
            createthread();

            processed = 0;
        }

        ~demothread_t()
        {
            join();

            SDL_DestroyMutex(mutex);
            SDL_DestroyCond(cond);

            if (pd.demoinfo)
                delete pd.demoinfo;
        }
    };

    int checkdemothread(void *data)
    {
        demothread_t *dt = (demothread_t*)data;

        while (dt->wait())
        {
            if (dt->jointhread)
            {
                dt->unlock();
                break;
            }

            dt->processresult = false;

            dt->unlock();

            dt->totaltime = parsedemo(&dt->pd);

            dt->lock();

            dt->processed++;
            dt->isprocessing = false;
            dt->processresult = true;

            dt->unlock();
        }

        return 0;
    }

    demothread_t *findnextavailthread(demothread_t **threads, int threadcount)
    {
        loopi(threadcount)
        {
            demothread_t *thread = *threads++;

            if (!thread->isactive())
                return thread;
        }

        return NULL;
    }

    void joinallthreads(demothread_t **threads, int &threadcount)
    {
        while (threadcount)
        {
            searchdemodbgcode(, consoleoutf("-", "joining threads (%d thread%s left...)",
                                            threadcount, plural(threadcount)));
            delete *threads++;
            threadcount--;
        }
    }

    void processthreadresult(demothread_t *thread, const char *mode, const extinfo::player &fp,
                             vector<searchdemoinfo*> &totalresult, int &readerrors, int &noextinfocount,
                             llong &totalfilesize, ullong &totaluncompressticks, ullong &totalparsedemoticks,
                             ullong &totalpacketcount, ullong &totalcputicks, const int &processnum, llong mindemosize = -1)
    {
        if (parsedemosuccess(thread->totaltime))
        {
            string matches;

            if (!thread->pd.result->empty())
            {
                searchdemoinfo &sdi = *totalresult.add(new searchdemoinfo);

                sdi.demo = thread->realdemofile;
                sdi.demoinfo = thread->pd.demoinfo;
                sdi.gamemode = mode;
                sdi.result = *thread->pd.result;
                formatstring(matches)(" - matches: %d", thread->pd.result->length());
            }
            else
            {
                delete thread->pd.demoinfo;
                delete[] thread->realdemofile;
                *matches = 0;
            }

            totalfilesize += *thread->pd.demofilesize;
            totaluncompressticks += *thread->pd.uncompressticks;
            totalparsedemoticks += *thread->pd.parsedemoticks;
            totalpacketcount += *thread->pd.packetcount;
            totalcputicks += thread->totaltime;

            consoleoutf("%s[%s] -> processed [%d/%d]%s", "took: %.3f ms (parsing: %.3f ms) (%u packets) (thread: %d)",
                       *matches ? "\f0" : "", mode, processnum, thread->filecount, matches, thread->totaltime/1000.0f,
                       (*thread->pd.parsedemoticks)/1000.0f, *thread->pd.packetcount, thread->threadid);

            thread->pd.result->setsize(0);
        }
        else
        {
            string error;
            const char *color;

            if (thread->totaltime == ERROR_DEMO_CONTAINS_NO_EXTINFO)
            {
                noextinfocount++;
                formatstring(error)("(%s)", thread->pd.error);
                color = "\f7";
            }
            else
            {
                formatstring(error)("error: (%s): %s", thread->realdemofile, thread->pd.error);
                readerrors++;
                color = "\f3";
            }

            totalpacketcount += *thread->pd.packetcount;

            consoleoutf("%s[%s] -> processed [%d/%d] - %s", "-", color, mode, processnum, thread->filecount, error);

            delete thread->pd.demoinfo;
            delete[] thread->realdemofile;
        }

        thread->reset();
    }

    void checkdemos(const char *dir, vector<char*> &files, const char *mode, const extinfo::player &fp,
                    vector<searchdemoinfo*> &totalresult, int &readerrors, int &noextinfocount, llong &totalfilesize,
                    ullong &totaluncompressticks, ullong &totalparsedemoticks, ullong &totalpacketcount, ullong &totalcputicks,
                    int gamemode = -1, const char *servername = NULL, const char *mapname = NULL, llong mindemosize = -1,
                    int maxthreads = MAXTHREADS)
    {
        int processnum = 0;
        int filecount = files.length();

        SDL_mutex *threadavailablemutex = SDL_CreateMutex();
        SDL_cond *threadavailablecond = SDL_CreateCond();

        if (!threadavailablemutex || !threadavailablecond)
            abort();

        demothread_t *threads[MAXTHREADS];
        int threadcount = min(guessnumcpus(), min(files.length(), maxthreads));
        loopi(threadcount) threads[i] = new demothread_t(fp, filecount, gamemode, servername, mapname, mindemosize, i+1, threadavailablecond);

        #define JOIN do { joinallthreads((checkdemo**)&threads, threadcount); } while(0)

        #define CHECKJOIN do { \
            { \
                SDL_Mutex_Locker m(searchmutex); \
                if (!jointhread) break; \
            } \
            \
            joinallthreads((demothread_t**)&threads, threadcount); \
            totalresult.deletecontents(); \
            files.deletearrays(); \
            return; \
        } while(0)

        consoleoutf("mode: %s found: %d demos (using %d thread%s)", "-",
                    strcmp(mode, "home") ? mode : "any", filecount, threadcount, plural(threadcount));

        loopvrev(files)
        {
            CHECKJOIN;

            demothread_t *thread;

            searchdemodbgcode(ullong findthreadstart = 0, findthreadstart = getmicroseconds());

            do
            {
                thread = findnextavailthread((demothread_t**)&threads, threadcount);

                if (!thread)
                {
                    SDL_mutexP(threadavailablemutex);
                    SDL_CondWait(threadavailablecond, threadavailablemutex);
                    SDL_mutexV(threadavailablemutex);
                }

                CHECKJOIN;
            } while (!thread);

            CHECKJOIN;

            searchdemodbgcode(, consoleoutf("-", "assigned thread %d (~%.3f ms)", thread->threadid, (getmicroseconds()-findthreadstart)/1000.0f));

            thread->lock();

            if (thread->needresultprocessing())
            {
                processthreadresult(thread, mode, fp, totalresult, readerrors, noextinfocount,
                                    totalfilesize, totaluncompressticks, totalparsedemoticks,
                                    totalpacketcount, totalcputicks, ++processnum, mindemosize);
            }

            char *demofile = files.remove(i);

            formatstring(thread->pd.demo)("%s/%s.dmo", dir, demofile);
            thread->realdemofile = demofile;
            thread->pd.demoinfo = new demoinfo_t;

            thread->unlock();

            while (!thread->iswaiting()) // this may happen on slower machines (thread not yet created)
                SDL_Delay(1);

            thread->signal();
        }

        int processedthreads = 0;

        do
        {
            loopi(threadcount)
            {
                demothread_t *thread = threads[i];

                if (!thread || thread->isactive())
                    continue;

                if (thread->needresultprocessing())
                {
                    processthreadresult(thread, mode, fp, totalresult, readerrors, noextinfocount,
                                        totalfilesize, totaluncompressticks, totalparsedemoticks,
                                        totalpacketcount, totalcputicks, ++processnum, mindemosize);
                }

                searchdemodbgcode(, consoleoutf("-", "joining thread %d", thread->threadid));

                delete thread;
                threads[i] = NULL;

                processedthreads++;
            }

            if (processedthreads == threadcount)
                break;

            SDL_Delay(1);
        } while (true);

        SDL_DestroyMutex(threadavailablemutex);
        SDL_DestroyCond(threadavailablecond);

        #undef JOIN
        #undef CHECKJOIN
    }

    void searchinalldemos(const extinfo::player &fp, vector<searchdemoinfo*> &totalresult,
                          int gamemode = -1, const char *servername = NULL, const char *mapname = NULL,
                          llong mindemosize = -1, int maxthreads = MAXTHREADS)
    {
        ullong ticks = getmicroseconds();

        int readerrors = 0;
        int noextinfocount = 0;
        llong totalfilesize = 0;
        ullong totaluncompressticks = 0;
        ullong totalparsedemoticks = 0;
        ullong totalpacketcount = 0;
        ullong totalcputicks = 0;

        #define CHECKJOIN \
        do { \
            SDL_Mutex_Locker m(searchmutex); \
            if (jointhread) return; \
        } while(0)

        loopi(NUMGAMEMODES)
        {
            string mode;

            copystring(mode, gamemodes[i].name);
            demorecorder::fixdemofilestring(mode);

            defformatstring(dir)("%s/%s", CLIENT_DEMO_DIRECTORY, mode);

            vector<char*> files;

            lockstream();
            distinctlistfiles(dir, "dmo", files);
            unlockstream();

            if (files.empty())
                continue;

            checkdemos(dir, files, gamemodes[i].name, fp, totalresult,
                       readerrors, noextinfocount, totalfilesize,
                       totaluncompressticks, totalparsedemoticks,
                       totalpacketcount, totalcputicks, gamemode,
                       servername, mapname, mindemosize, maxthreads);

            CHECKJOIN;
        }

        CHECKJOIN;

        // process home directory

        vector<char*> files;

        lockstream();
        distinctlistfiles(".", "dmo", files);
        unlockstream();

        if (!files.empty())
        {
            consoleout("processing home directory", "");

            checkdemos(".", files, "home", fp, totalresult,
                       readerrors, noextinfocount, totalfilesize,
                       totaluncompressticks, totalparsedemoticks,
                       totalpacketcount, totalcputicks, gamemode,
                       servername, mapname, mindemosize, maxthreads);
        }


        CHECKJOIN;

        float filesize = totalfilesize/1024.0f/1024.0f;
        float realtotaltime = (getmicroseconds()-ticks)/1000000.0f;
        float cputime = totalcputicks/1000000.0f;

        consoleoutf("processed %.2f mb (%.2f mb/s) of demos! found matches "
                    "in %d demos, took: %.3f sec.", "(%llu packets) (cpu time: %.3f sec)",
                    filesize, filesize/(realtotaltime!=0.00f?realtotaltime:1.00f),
                    totalresult.length(), realtotaltime, totalpacketcount, cputime);

        #undef CHECKJOIN
    }

    static int comparedemo_date_desc(const searchdemoinfo *a, const searchdemoinfo *b)
    {
        demoinfo_t *dia = a->demoinfo;
        demoinfo_t *dib = b->demoinfo;
        if (dia == dib) return 0; // both NULL?
        if (!dia) return -1;
        if (!dib) return 0;
        if (dia->time > dib->time) return -1; // newer?
        if (dia->time < dib->time) return 0; // older?
        return 0; // same time..?
    }

    static int compareplayer_frags_desc(const extinfo::player *a, const extinfo::player *b)
    {
        if (a->frags > b->frags) return -1;
        if (a->frags < b->frags) return 0;
        return 0;
    }

    void searchdemocubescript(int gamemode, const char *servername, const char *mapname, const char *name,
                              int frags, int deaths, int acc, int teamkills, int flags, int priv, int maxresults = -1,
                              llong mindemosize = -1, int maxthreads = MAXTHREADS)
    {
        extinfo::player fp;

        fp.frags = frags;
        fp.deaths = deaths;
        fp.acc = acc;
        fp.teamkills = teamkills;
        fp.flags = flags;
        fp.priv = priv;

        fp.ping = -1;

        if (name && *name)
            copystring(fp.name, name, sizeof(fp.name));
        else
            *fp.name = 0;

        vector<searchdemoinfo*> result;
        searchinalldemos(fp, result, gamemode, servername, mapname, mindemosize, maxthreads);

        #define CHECKJOIN \
        do { \
            SDL_Mutex_Locker m(searchmutex); \
            if (jointhread) return; \
        } while(0)

        CHECKJOIN;

        if (!result.empty())
        {
            result.sort(comparedemo_date_desc);

            vector<char> cubescript;
            cubescript.reserve(200*1024);

            #define PUTSTRING(str) cubescript.add(str, strlen(str));

            PUTSTRING("newgui \"search demo result\" [\n");

            int matches = 0;

            loopv(result)
            {
                if (i > 0)
                    PUTSTRING("guibar\n");

                if (maxresults >= 0 && matches >= maxresults)
                {
                    consoleoutf("\f3warning:\f7 result is too big, please be more specific. showing only first %d matches.", "-", maxresults);
                    break;
                }

                string tmp;
                strtool escapebuf;
                searchdemoinfo &sdi = *result[i];

                if (sdi.demoinfo && sdi.demoinfo->ok) // show some extended informations when available
                {
                    demoinfo_t *di = sdi.demoinfo;

                    struct tm *tm;
#ifndef WIN32
                    struct tm tmtmp;
                    tm = localtime_r(&di->time, &tmtmp);
#else
                    tm = localtime(&di->time);
#endif
                    string timebuf;
                    string server;

                    if (!tm)
                        goto timeerror;

                    strftime(timebuf, sizeof(string), "%x %X", tm);

                    if (!*di->servinfo)
                    {
                        if (!di->host)
                            goto nohost;

                        ENetAddress address;
                        address.host = di->host;
                        address.port = di->port;

                        string serverip;

                        if (enet_address_get_host_ip(&address, serverip, sizeof(string)) < 0)
                            copystring(tmp, "-");

                        formatstring(server)("(%s:%u)", serverip, di->port);
                    }
                    else
                    {
                        nohost:;
                        copystring(server, di->servinfo);
                    }

                    string modename;
                    copystring(modename, m_valid(di->mode) ? gamemodes[di->mode-STARTGAMEMODE].name : "unknown");

                    defformatstring(demoinfostr)("Server: %s Mode: %s Map: %s  (%s)",
                                                 server, modename, di->map, timebuf);

                    formatstring(tmp)("guibutton %s ", escapecubescriptstring(demoinfostr, escapebuf));
                    PUTSTRING(tmp);
                }
                else
                {
                    timeerror:;

                    formatstring(tmp)("guibutton %s ", escapecubescriptstring(sdi.demo, escapebuf));
                    PUTSTRING(tmp);
                }

                string mode;
                copystring(mode, sdi.gamemode);
                demorecorder::fixdemofilestring(mode);

                escapebuf.clear();

                if (!strcmp(mode, "home"))
                    formatstring(tmp)("\"demo ^\"%s^\"\"\n", escapecubescriptstring(sdi.demo, escapebuf, false));
                else
                    formatstring(tmp)("\"demo ^\"%s/%s/%s^\"\"\n", CLIENT_DEMO_DIRECTORY, mode, escapecubescriptstring(sdi.demo, escapebuf, false));

                PUTSTRING(tmp);

                sdi.result.sort(compareplayer_frags_desc);

                loopvj(sdi.result)
                {
                    extinfo::player &p = *sdi.result[j];
                    escapebuf.clear();

#ifdef ENABLE_IPS
                    static const char *fmt = "Name: %s (%u)  Frags: %d  Deaths: %d  Acc: %d%%  TKs: %d  IP: %u.%u.%u";
#else
                    static const char *fmt = "Name: %s (%u)  Frags: %d  Deaths: %d  Acc: %d%%  TKs: %d";
#endif

                    PUTSTRING("guitext \"");
                    formatstring(tmp)(fmt, escapecubescriptstring(p.getname(), escapebuf, false), p.cn, p.frags,
                                      p.deaths, p.acc, p.teamkills, p.ip.ia[0], p.ip.ia[1], p.ip.ia[2]);
                    PUTSTRING(tmp);

                    PUTSTRING("\" \"\"\n");
                    matches++;
                }

                PUTSTRING("guibar\n");
            }

            PUTSTRING("\n]\n"); // newgui
            PUTSTRING("showgui \"search demo result\"");

            cubescript.add(0);

            execute_r(cubescript.getbuf(), 0, true);
            cubescript.disown();

            result.deletecontents();

            #undef PUTSTRING
        }
    }

    int searchthread(void *data)
    {
        searchdemothreadinfo *sdti = (searchdemothreadinfo*)data;

        {
            SDL_Mutex_Locker m(searchmutex);
            jointhread = false;
        }

        searchdemocubescript(sdti->gamemode, sdti->servername, sdti->mapname,
                             sdti->name, sdti->frags, sdti->deaths, sdti->acc,
                             sdti->teamkills, sdti->flags, sdti->priv,
                             sdti->maxresults, sdti->mindemosize, sdti->maxthreads);

        delete sdti;

        {
            SDL_Mutex_Locker m(searchmutex);
            jointhread = true;
        }

        return 0;
    }

    void searchdemo(const char *gamemode, const char *servername, const char *mapname,
                    const char *name, int *frags, int *deaths, int *acc, int *teamkills,
                    int *flags, int *priv)
    {
        if (searchdemothread)
        {
            consoleout("already searching for demos, type '/stopsearchdemo' to stop searching", "-");
            return;
        }

        searchdemothreadinfo *sdti = new searchdemothreadinfo;

        if ((sdti->gamemode = gamemod::getgamemodenum(gamemode)) == STARTGAMEMODE-1)
        {
            erroroutf_r("invalid gamemode given");
            return;
        }

        strtool sname(sdti->servername, sizeof(sdti->servername));
        sname = servername;
        sname.lowerstring();

        strtool mname(sdti->mapname, sizeof(sdti->mapname));
        mname = mapname;
        mname.lowerstring();

        copystring(sdti->name, name);

        sdti->frags = *frags;
        sdti->deaths = *deaths;
        sdti->acc = *acc;
        sdti->teamkills = *teamkills;
        sdti->flags = *flags;
        sdti->priv = *priv;

        sdti->maxresults = searchdemomaxresults;
        sdti->mindemosize = searchdemomindemosize*1024;

        sdti->maxthreads = searchdemothreadlimit;

        searchdemothread = SDL_CreateThread(&searchthread, sdti);

        if (!searchdemothread)
            abort();
    }
    COMMAND(searchdemo, "ssssiiiiii");

    ICOMMAND(stopsearchdemo, "", (), {
        if (!searchdemothread || !searchmutex)
            return;

        {
            SDL_Mutex_Locker m(searchmutex);

            if (jointhread)
                return;

            jointhread = true;
        }

        consoleout("sending stop signal...", "-");
    });

    void stripextdemo(const char *demo)
    {
        llong demofilesize;

        defformatstring(demofile)("%s.dmo", demo);
        defformatstring(newfile)("%s.tmp", demofile);

        stripdemo_t stripdemo;
        stripdemo.strippedfile = opengzfile(newfile, "wb", NULL, clientdemocompresslevel);
        stripdemo.written = 0;
        stripdemo.skippacketcount = 0;

        if (!stripdemo.strippedfile)
        {
            consoleoutf("couldn't open (%s) for writing.", "-", newfile);
            return;
        }

        int totaltime;
        uint packetcount;
        string error;

        parsedemo_t pd;
        pd.demo = demofile;
        pd.packetcount = &packetcount;
        pd.demofilesize = &demofilesize;
        pd.mindemosize = 0;
        pd.stripdemo = &stripdemo;
        pd.error = error;

        if (parsedemosuccess((totaltime = parsedemo(&pd))))
        {
            delete stripdemo.strippedfile;

            if (remove(demofile) || rename(newfile, demofile))
            {
                consoleoutf("couldn't rename/remove file", "-");
                return;
            }

            consoleoutf("stripped %.3f kb (%u/%u packets) from demo (%s) (%.2f sec)", "-",
                       (demofilesize-stripdemo.written)/1024.0f, stripdemo.skippacketcount, packetcount,
                       demofile, totaltime/1000.0f/1000.0f);

            return;
        }

        delete stripdemo.strippedfile;
        remove(newfile);
        consoleoutf("failed to strip demo (%s) (packetnum: %u)", "-", error, packetcount);
    }
    COMMAND(stripextdemo, "s");

    void recompressdemo(const char *demo)
    {
        size_t size;
        char *buf = NULL;
        void *udata = NULL;
        size_t ulen;
        float diff;

        defformatstring(demofile)("%s.dmo", demo);
        defformatstring(newfile)("%s.tmp", demofile);

        stream *f = opengzfile(newfile, "wb", NULL, clientdemocompresslevel);

        if (!(buf = loadfile(demofile, &size, false)))
        {
            consoleoutf("couldn't open (%s) for reading", "-", demofile);
            goto cleanup;
        }

        if (!f)
        {
            consoleoutf("couldn't open (%s) for writing", "-", newfile);
            goto cleanup;
        }

        if (uncompress(buf, size, &udata, ulen, 200*1024*1024) != UNCOMPRESS_OK)
        {
            consoleout("couldn't uncompress demo.", "-");
            goto cleanup;
        }

        if (f->write(udata, ulen) != ulen)
        {
            consoleout("couldn't write new file.", "-");
            goto cleanup;
        }

        f->close();

        if (remove(demofile) || rename(newfile, demofile))
        {
            consoleoutf("couldn't rename/remove file", "-");
            goto cleanup;
        }

        delete f;
        f = openfile(demofile, "rb"); // get compressed size

        if (!f)
            goto cleanup;

        f->seek(0, SEEK_END);
        diff = (f->tell()-stream::offset(size))/1024.0f;
        consoleoutf("recompressed demo (%s) (%s%.2f kb) successfully", "-", demo, diff >= 0.0f ? "+" : "", diff);

        cleanup:;

        delete f;
        delete[] buf;
        free(udata);
    }
    COMMAND(recompressdemo, "s");

    void recompressdemos(const char *dir)
    {
        vector<char*> files;
        distinctlistfiles(dir, "dmo", files);

        if (files.empty())
        {
            consoleoutf("couldn't find any demos in (%s)", "-", dir);
            return;
        }

        loopv(files)
        {
            defformatstring(demo)("%s/%s", dir, files[i]);
            recompressdemo(demo);
        }

        files.deletearrays();
    }
    COMMAND(recompressdemos, "s");

    void getdemoduration(const char *demo)
    {
        string error;

        int demomillis;
        uint packetnum;

        defformatstring(demofile)("%s.dmo", demo);

        parsedemo_t pd;

        pd.demo = demofile;
        pd.packetcount = &packetnum;
        pd.demomillis = &demomillis;
        pd.error = error;

        if (parsedemosuccess(parsedemo(&pd)))
        {
            intret(demomillis);
            return;
        }

        consoleoutf("failed to read demo length (%s) (packetnum: %u)", "-", error, packetnum);
        intret(-1);
    }
    COMMAND(getdemoduration, "s");

    void startup()
    {
        searchmutex = SDL_CreateMutex();

        if (!searchmutex)
            fatal("couldn't create search demo mutex");
    }

    void shutdown()
    {
        if (searchdemothread)
        {
            consoleout("waiting for searchdemo thread(s)...", "-");

            {
                SDL_Mutex_Locker m(searchmutex);
                jointhread = true;
            }

            slice();
        }

        SDL_DestroyMutex(searchmutex);
        searchmutex = NULL;
    }

    void slice()
    {
        if (!searchdemothread)
            return;

        {
            SDL_Mutex_Locker m(searchmutex);

            if (!jointhread)
                return;
        }

        SDL_WaitThread(searchdemothread, NULL);
        searchdemothread = NULL;

        SDL_Mutex_Locker m(searchmutex);
        jointhread = false;
    }

    #undef MAXTHREADS
} // namespace demorecorder
} // namespace mod
