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

#ifndef _DEMORECORDER_H_
#define _DEMORECORDER_H_

static const int N_DEMORECORDER_EXTINFO_SERVER  = -12345;
static const int N_DEMORECORDER_DEMOINFO_SERVER = -12346;

static const int N_DEMORECORDER_EXTINFO_INT  = 781123017;
static const int N_DEMORECORDER_DEMOINFO_INT = 199695799;

static const int N_DEMORECORDER_EXTINFO_EXT  = 898126499;
static const int N_DEMORECORDER_DEMOINFO_EXT = 258864235;

namespace mod {
namespace demorecorder
{
    static const char *const CLIENT_DEMO_DIRECTORY = "demos";

    extern bool demorecord;
    extern char *demofile;
    extern bool parsingmapchangepacket;
    extern int servmsgcmdignoremillis;

    void fixdemofilestring(char *str);
    void writedemo(int chan, void *data, int len, bool nostamp = false, int offsetlen = 0);

    struct demoinfo_t;

    extern int clientdemorecordextinfo;
    extern int clientdemoskipservmsg;
    extern int clientdemoskipservcmds;
    extern int clientdemoautorecord;
    extern int clientdemocompresslevel;

    namespace self
    {
        void switchname(const char *newname);
        void text(const char *text);
        void gunselect(int gun);
        void shoteffect(void *d, int gun, int id, const vec &from, const vec &to);
        void sound(void *d, int n);
        void ping(int n);
        void posupdate(void *d, void *data, int len);
        void jumppad(int jp, void *d);
        void teleport(int n, int td, void *d);

        void putextinfoobj(extinfo::playerv2 *ep);
        extinfo::playerv2 *getextinfoobj(ucharbuf &p, extinfo::playerv2 *buf = NULL);

        void putdemoinfoobj(ucharbuf &p);
        demoinfo_t* getdemoinfoobj(ucharbuf &p, demoinfo_t *demoinfo);
    }

    void startup();
    void shutdown();

    const char *genproperdemofilename(char *buf, size_t len);
    void setupdemorecord(const char *name = NULL, bool force = false);
    void stopdemorecord();

    class skip_packet_
    {
    public:
        skip_packet_(ucharbuf &pb, int packettype) : initlen(pb.length()), p(pb)
        {
            if (packettype < 128 && packettype > -127)
                initlen--;
            else if (packettype < 0x8000 && packettype >= -0x8000)
                initlen -= 3; // 0x80 + 2 bytes
            else
                initlen -= 5; // 0x81 + 4 bytes
        }
        ~skip_packet_()
        {
            int bytes_to_skip = p.length()-initlen;
            p.len -= bytes_to_skip;
            p.maxlen -= bytes_to_skip;
            if (p.maxlen) // there are packets left, need to move buffer
                memmove(p.buf+p.length(), p.buf+p.length()+bytes_to_skip, p.maxlen-p.length());
        }
    private:
        int initlen;
        ucharbuf &p;
    };

    class skip_packet
    {
    public:
        skip_packet(ucharbuf &p, int packettype, bool cond = true) : sp(NULL)
        {
            if (!demorecord || !cond)
                return;
            sp = new skip_packet_(p, packettype);
        }
        ~skip_packet()
        {
            if (sp) delete sp;
        }
    private:
        skip_packet_ *sp;
    };

    #define DEMORECORDER_SKIP_PACKET(c) mod::demorecorder::skip_packet sp(p, type, !!(c))
    #define DEMORECORDER_SKIP_PACKET_NC mod::demorecorder::skip_packet sp(p, type)
    #define DEMORECORDER_SKIP_PACKET_FUNC(c, f) bool cond = !!(c); if (cond) f; mod::demorecorder::skip_packet sp(p, type, cond)

    struct demoinfo_t
    {
        time_t time;
        uint host;
        uint16_t port;
        string servinfo;
        int mode;
        string map;
        bool ok;
        demoinfo_t() { ok = false; }
    };
}

namespace searchdemo
{
    using namespace demorecorder;

    struct searchdemoinfo
    {
        const char *gamemode;
        char *demo;
        vector<extinfo::playerv2*> result;
        demorecorder::demoinfo_t *demoinfo;

        searchdemoinfo() : demoinfo(NULL) { }

        ~searchdemoinfo()
        {
            result.deletecontents();
            delete[] demo;

            if (demoinfo)
                delete demoinfo;
        }
    };

    struct searchdemothreadinfo
    {
        int gamemode;
        string servername;
        string mapname;
        string name;

        int frags, deaths, acc;
        int teamkills, flags, priv;

        int maxresults;
        llong mindemosize;

        int maxthreads;
    };

    struct stripdemo_t
    {
        stream *strippedfile;
        int compression;
        uint position;
        void *buf;
        size_t written;
        uint skippacketcount;

        void write(size_t len, int chan, int nextplayback)
        {
            int stamp[3] = { nextplayback, chan, (int)len };
            lilswap(stamp, 3);
            written += strippedfile->write(stamp, sizeof(stamp));
            written += strippedfile->write((char*)buf+position, (int)len);
        }

        void write(const void *data, size_t len)
        {
            written += strippedfile->write(data, (int)len);
        }

        void write(size_t len)
        {
            written += strippedfile->write((char*)buf+position, (int)len);
        }
    };

    struct parsedemo_t
    {
        const char *demo;
        const extinfo::playerv2 *fp;
        vector<extinfo::playerv2*> *result;
        demorecorder::demoinfo_t *demoinfo;
        llong *demofilesize;
        ullong *uncompressticks;
        ullong *parsedemoticks;
        uint *packetcount;
        int gamemode;
        const char *servername;
        const char *mapname;
        llong mindemosize;
        char *error;
        stripdemo_t *stripdemo;
        int *demomillis;

        parsedemo_t()
        {
            memset(this, 0, sizeof(*this));
            gamemode = -1;
        }
    };

    llong parsedemo(parsedemo_t *pd);

    enum
    {
        ERROR_FAILED_TO_OPEN_DEMO_FILE = -1,
        ERROR_FAILED_TO_READ_DEMO_FILE = -2,
        ERROR_DEMO_FILE_TOO_BIG = -4,
        ERROR_INVALID_UNCOMPRESS_SIZE = -4,
        ERROR_OUT_OF_MEMORY = -5,
        ERROR_FAILED_TO_INIT_ZLIB = -6,
        ERROR_FAILED_TO_UNCOMPRESS_DEMO = -7,
        ERROR_INVALID_DEMO_HEADER = -8,
        ERROR_INVALID_DEMO_VERSION = -9,
        ERROR_INVALID_PACKET_SIZE = -10,
        ERROR_FAILED_TO_READ_PACKET = -11,
        ERROR_FAILED_TO_JUMP_BUF = -12,
        ERROR_DEMO_CONTAINS_NO_EXTINFO = -13
    };

    static inline bool parsedemosuccess(llong val) { return val >= 0; }

    void startup();
    void shutdown();
    void slice();
} // namespace demorecorder
} // namespace mod

#endif //!_DEMORECORDER_H_
