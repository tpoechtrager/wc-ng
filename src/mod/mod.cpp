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

#include "cube.h"
#include <curl/curl.h>
#include LIB_GEOIP_HEADER
#include "../last_sauer_svn_rev.h"

#include <openssl/ssl.h>

#ifdef __APPLE__
#include <CoreServices/CoreServices.h>
#include <mach/mach.h>
#include <mach/mach_time.h>
#endif

static struct
{
    const char *const URL;
    const char *const CACERT;
} UPDATE_CHECK_URLS[] =
{
    { "https://wc-ng.sauerworld.org/checkversion.php", "data/CA/wc-ng-ca.crt" }
};

// engine
extern void connectserv(const char *servername, int port, const char *serverpassword);

namespace mod
{
#ifndef STANDALONE
    struct cubescript
    {
        char *code;
        void (*callback)(int, void *);
        void *callbackarg;
        bool disowned;
        uint id;
    };

    struct threadmessage
    {
        strtool msg;
        int con;
        threadmessage(const char *msg, int con) : msg(msg), con(con) {}
        threadmessage(strtool &msg, int con) : con(con) { this->msg.swap(msg); }
    };

    SDL_mutex *threadmutex = NULL;
    vector<threadmessage*> threadmessages;
    vector<cubescript> threadscripts;
    atomic<Uint32> mainthreadid(-1);

    bool ismainthread()
    {
        if (SDL_ThreadID() == mainthreadid || mainthreadid == (Uint32)-1)
            return true;

        return false;
    }

    void addthreadmessage(strtool &msg, int con)
    {
        if (ismainthread())
        {
            conoutf(con, "%s", msg.str());
            return;
        }

        SDL_Mutex_Locker m(threadmutex);
        threadmessages.add(new threadmessage(msg, con));
    }

    void conoutf_r(int con, const char *fmt, ...)
    {
        strtool msg(512);

        va_list v;
        va_start(v, fmt);
        msg.vfmt(fmt, v);
        va_end(v);

        addthreadmessage(msg, con);
    }

    void conoutf_r(const char *fmt, ...)
    {
        strtool msg(512);

        va_list v;
        va_start(v, fmt);
        msg.vfmt(fmt, v);
        va_end(v);

        addthreadmessage(msg);
    }

    void erroroutf_r(const char *fmt, ...)
    {
        strtool msg(512);
        msg.copy("\f3error:\f7 ", 11);

        va_list v;
        va_start(v, fmt);
        msg.vfmt(fmt, v);
        va_end(v);

        addthreadmessage(msg, CON_ERROR);
    }

    static inline void printthreadmessages()
    {
        loopv(threadmessages)
        {
            threadmessage *tm = threadmessages[i];
            conoutf(tm->con, "%s", tm->msg.str());
        }

        threadmessages.deletecontents();
    }

    uint execute_r(const char *code, size_t codelen, bool disown,
                   void (*callback)(int, void *), void *callbackarg,
                   int *mainresult)
    {
        if (ismainthread())
        {
            int res = execute(code);

            if (callback)
                callback(res, callbackarg);

            if (mainresult)
                *mainresult = res;

            if (disown)
                delete[] (uchar*)code;

            return EXECUTE_R_EXECUTED_IN_MAIN;
        }

        uint csid = -1;

        {
            SDL_Mutex_Locker m(threadmutex);

            cubescript &cs = threadscripts.add();

            cs.disowned = disown;

            if (disown)
                cs.code = (char*)code;
            else if (codelen)
                cs.code = newstring(code, codelen);
            else
                cs.code = newstring(code);

            cs.callback = callback;
            cs.callbackarg = cs.callbackarg;
            static volatile uint nextid = 0;

            if (nextid == EXECUTE_R_EXECUTED_IN_MAIN)
                nextid++;

            cs.id = nextid++;
            csid = cs.id;
        }

        return csid;
    }

    int remove_execute_r(uint id)
    {
        int res = 0;

        {
            SDL_Mutex_Locker m(threadmutex);

            loopvrev(threadscripts)
            {
                cubescript &cs = threadscripts[i];
                if (cs.id == id)
                {
                    cubescript cs = threadscripts.remove(i);
                    delete[] cs.code;
                    threadscripts.remove(i);
                    if (!cs.disowned) delete[] cs.code;
                    else delete[] (uchar*)cs.code;
                    res = EXECUTE_R_REMOVE_SUCCESS;
                    break; //mutex
                }
            }
        }

        if (!res)
            res = EXECUTE_R_INVALID_ID;

        return res;
    }

    static inline void executethreadscripts()
    {
        loopv(threadscripts)
        {
            cubescript &cs = threadscripts[i];

            int res = execute(cs.code);
            delete[] cs.code;

            if (cs.callback)
                cs.callback(res, cs.callbackarg);
        }

        threadscripts.setsize(0);
    }

#ifdef WIN32
    static ullong clockspeed;

    struct initnanotimer
    {
        llong getclockspeed()
        {
            LARGE_INTEGER speed;

            if (!QueryPerformanceFrequency(&speed))
                speed.QuadPart = 0;

            if (speed.QuadPart <= 0)
                abort();

            return speed.QuadPart;
        }

        initnanotimer() {  clockspeed = getclockspeed(); }
    } initnanotimer;
#endif //WIN32

    static ullong nanosecbase = 0;
    void setnanosecondsbase(ullong baseval) { nanosecbase = baseval; }

    ullong getnanoseconds()
    {
#ifdef WIN32
        ullong ticks;
        if (!QueryPerformanceCounter((LARGE_INTEGER*)&ticks))
            abort();
        double tmp = (ticks/(double)clockspeed)*1000000000.0;
        return tmp-nanosecbase;
#else
#ifndef __APPLE__
        struct timespec tp;
        struct timeval tv;
        if (clock_gettime(CLOCK_MONOTONIC, &tp) == 0)
            return (ullong)((tp.tv_sec*1000000000LL)+tp.tv_nsec)-nanosecbase;
        if (gettimeofday(&tv, NULL) == 0)
            return (ullong)((tv.tv_sec*1000000000LL)+(tv.tv_usec*1000))-nanosecbase;
        abort();
#else
        union
        {
            AbsoluteTime at;
            ullong ull;
        } tmp;
        tmp.ull = mach_absolute_time();
        Nanoseconds ns = AbsoluteToNanoseconds(tmp.at);
        tmp.ull = UnsignedWideToUInt64(ns)-nanosecbase;
        return tmp.ull;
#endif //!__APPLE__
#endif //WIN32
    }

    uint getsaltedmicroseconds(bool newsalt, uint saltedmicroseconds, int *salt,
                               void (*seed)(uint), uint (*rand)())
    {
        static int microsecondssalt = 0;

        if (!salt) salt = &microsecondssalt;

        if (saltedmicroseconds != (uint)-1)
        {
            uint result = (getmicroseconds()%0x7FFFFFFE)-(saltedmicroseconds-*salt);

            if (result >= 0x7FFFFFFE)
                return 0;

            return result;
        }

        if (!*salt || newsalt)
        {
#ifdef __clang__
#if __has_builtin(__builtin_readcyclecounter)
            auto v = __builtin_readcyclecounter();

            if (v)
            {
                seed(v);
                goto ok;
            }
#endif
#endif
            seed((getmicroseconds()%0xFFFFFFFF)+(time(NULL)%0x7FFFFFFF));
#ifdef __clang__
            ok:;
#endif
            *salt = rand()%0x7FFFFFFE;
        }

        return (getmicroseconds()%0x7FFFFFFE)+*salt;
    }

    void dumppacketbuf(ucharbuf &p)
    {
        int i = 0;
        while (!p.overread())
            conoutf_r("%d: %d", ++i, getint(p));
    }

    int distinctlistfiles(const char *dir, const char *ext, vector<char*>& files)
    {
        listfiles(dir, ext, files);
        loopvrev(files)
        {
            char *file = files[i];
            bool redundant = false;
            loopj(i) if(!strcmp(files[j], file)) { redundant = true; break; }
            if(redundant) delete[] files.removeunordered(i);
        }
        return files.length();
    }

    struct timer
    {
        void *cbdata;
        void *callback;
        int type;
        int timeadded;
        int last;
        int interval;
        bool once;
        uint id;
    };

    static uint nexttimerid = 0;
    static vector<timer> timers;

    uint addtimerfunction(int interval, bool once, void *callback, int type, void *cbdata)
    {
        timer &timer = timers.add();

        timer.cbdata = cbdata;
        timer.callback = callback;
        timer.type = type;
        timer.timeadded = totalmillis;
        timer.interval = interval;
        timer.last = totalmillis;
        timer.once = once;
        timer.id = nexttimerid++;

        return timer.id;
    }

    bool removetimerfunction(uint id, void (*freecallback)(void *cbdata))
    {
        loopvrev(timers)
        {
            timer &timer = timers[i];

            if (timer.id == id)
            {
                void *cbdata = timers.remove(i).cbdata;

                if (cbdata && freecallback)
                    freecallback(cbdata);

                return true;
            }
        }

        return false;
    }

    static inline void runtimerfunctions()
    {
        loopvrev(timers)
        {
            timer &timer = timers[i];

            if (totalmillis-timer.last >= timer.interval)
            {
                switch (timer.type)
                {
                    case timerfunctype<timerfunctype1>::type:
                    {
                        ((timerfunctype1)timer.callback)();
                        break;
                    }
                    case timerfunctype<timerfunctype2>::type:
                    {
                        ((timerfunctype2)timer.callback)(timer.cbdata, timer.id);
                        break;
                    }
                    default: fatal("unhandled timer function type");
                }

                timer.last = totalmillis;

                if (timer.once)
                    timers.remove(i);
            }
        }
    }

    static void freebandwidthdata(void **data)
    {
        if (*data)
        {
            delete (clientbwstats*)*data;
            *data = NULL;
        }
    }

    void calcclientbandwidthstats()
    {
        if (!curpeer)
            return;

        if (!curpeer->bandwidthStats.data)
        {
            curpeer->bandwidthStats.data = new clientbwstats;
            curpeer->bandwidthStats.freecallback = freebandwidthdata;
        }

        realBandwidthStats &rbws = curpeer->bandwidthStats;
        clientbwstats *bw = (clientbwstats*)curpeer->bandwidthStats.data;

        int timeelapsed = totalmillis-bw->millis;

        if (timeelapsed >= 1000)
        {
            float factor = timeelapsed/1000.0f;

            bw->curincomingkbs = (rbws.incomingDataTotal-bw->last.incomingDataTotal)/1024.0f/factor;
            bw->curoutgoingkbs = (rbws.outgoingDataTotal-bw->last.outgoingDataTotal)/1024.0f/factor;

            bw->curincomingpacketssec = (rbws.packetsReceived-bw->last.packetsReceived)/factor;
            bw->curoutgoingpacketssec = (rbws.packetsSent-bw->last.packetsSent)/factor;

            bw->incomingtotal = rbws.incomingDataTotal/1024.0f;
            bw->outgoingtotal = rbws.outgoingDataTotal/1024.0f;

            bw->last = curpeer->bandwidthStats;
            bw->millis = totalmillis;

            event::run(event::BANDWIDTH_UPDATE, "ffddffddd",
                       bw->curincomingkbs, bw->curoutgoingkbs,
                       bw->curincomingpacketssec, bw->curoutgoingpacketssec,
                       bw->incomingtotal, bw->outgoingtotal,
                       rbws.packetsReceived, rbws.packetsSent,
                       bw->last.packetsLost);
        }
    }

    static bool checkingversion = false;
    static int lastversioncheck = 0;

    void checkversioncallback(uint requestid, int *cbdata, bool ok,
                              int responsecode, const char *buf, size_t datalen)
    {
        strtool data(buf, datalen);
        strtool *strs = NULL;
        size_t n;
        size_t verindex = 0;
        clientversion ver;

        /*
         * Server response format:
         *
         * [<revoked chat certs>]
         * <version>
         * [<changelog>]
         */

        data.convert2cube();

        if (!ok)
        {
            if (cbdata[0] /* user request */)
            {
                if (responsecode == http::SSL_ERROR) erroroutf_r("can't verify ssl certificate");
                else erroroutf_r("update check server unreachable");
            }

            goto fail;
        }

        if (responsecode != 200)
        {
            if (cbdata[0] /* user request */)
            {
                erroroutf_r("update check server replied with an "
                            "invalid http response code '%d'", responsecode);
            }

            goto fail;
        }

        if (!(n = data.split("\n", &strs, true)))
        {
            if (cbdata[0] /* user request */)
                erroroutf_r("invalid response from update check server");

            goto fail;
        }

        if (strs[0].length() > 10)
        {
            if (n < 2)
                goto fail;

            verindex++;

            strtool *revokedcerts = NULL;
            size_t n;

            if (!(n = strs[0].split(" ", &revokedcerts, true)))
                goto fail;

            loopi(n)
                chat::addrevokedcert(revokedcerts[i].str());

            delete[] revokedcerts;
        }

        ver = parseclientversion(strs[verindex].str());

        if (ver > CLIENTVERSION)
        {
            conoutf("there is a new version of WC NG available for download");

            conoutf("new version: %s  your version: %s",
                    ver.str().str(),
                    CLIENTVERSION.str().str());

            if (n >= verindex+2)
            {
                conoutf("changes:");

                for (size_t i=++verindex; i<n; i++)
                    conoutf("%s", strs[i].str());
            }
        }
        else if (cbdata[0] /* user request */)
        {
            conoutf("you are using the latest version of WC NG");
        }

        lastversioncheck = totalmillis;
        goto cleanup;

        fail:;

        if (++(cbdata[1]) < (int)sizeofarray(UPDATE_CHECK_URLS))
        {
            checkingversion = false;

#if 0
            if (cbdata[0] /* user request */)
                conoutf("trying next update check server");
#endif

            wccheckversion(!!cbdata[0], cbdata[1]);
        }

        cleanup:;
        checkingversion = false;
        delete[] strs;
        delete[] cbdata;
    }

    void wccheckversion(bool userrequest, int n)
    {
        if (checkingversion) // allow only one instace at a time
        {
            conoutf("already checking for newer version");
            return;
        }

        if (lastversioncheck)
        {
            static const int checklimit = 5000;
            int timeleft = (totalmillis-lastversioncheck-checklimit)*-1;

            if (timeleft >= 1)
            {
                conoutf("please wait %.2f seconds before doing another check", timeleft/1000.0f);
                return;
            }
        }

        checkingversion = true;

        const auto &mirror = UPDATE_CHECK_URLS[n];

        strtool url;
        url << mirror.URL << "?v=" << CLIENTVERSION.num() << "&r=" << getwcrevision();

        int *data = new int[2];

        data[0] = userrequest;
        data[1] = n;

        auto *httpreq = new http::request_t;

        copystrtool(httpreq->request, url);
        if (mirror.CACERT) httpreq->cacert = mirror.CACERT;
        httpreq->callbackdata = data;
        httpreq->callback = (http::callback_t)checkversioncallback;

        if (http::request(httpreq) == http::UNABLE_TO_PROCESS_REQUEST_NOW)
        {
            if (userrequest) conoutf("can't check for updates");
        }
    }

    static void wcver(const char *op, const char *ver)
    {
        clientversion v;
        if (*ver) v = parseclientversion(ver);

        #define res(expr) do { intret((expr)); return; } while (0)

        if (!strcmp(op, ">=")) res(CLIENTVERSION >= v);
        else if (!strcmp(op, "<=")) res(CLIENTVERSION <= v);
        else if (!strcmp(op, "==")) res(CLIENTVERSION == v);
        else if (!strcmp(op, "<")) res(CLIENTVERSION < v);
        else if (!strcmp(op, ">")) res(CLIENTVERSION > v);
        else if (!strcmp(op, "!=")) res(CLIENTVERSION != v);

        #undef res

        static auto verbuf = CLIENTVERSION.str();

        if (*op == '1')
        {
            conoutf("%s", verbuf.str());
            return;
        }

        result(verbuf.str());
    }

    COMMAND(wcver, "ss");
    VAR(wcversion, 1200, 1200, 1200); // compat
    ICOMMAND(wccheckversion, "", (), wccheckversion(true));
    MODVARP(wcautoversioncheck, 0, 1, 1);

    void wcautocheckversion() // needs to be called after config.cfg was executed
    {
        if (!wcautoversioncheck)
            return;

        wccheckversion();
    }

    static void wcinfo(int *get)
    {
        static const char *const *compiler = getcompilerversion();
        static const char *const wcrev = getwcrevision();

        static strtool info;
        static strtool libinfo;

        if (!info)
        {
            info << "WC NG version: " << CLIENTVERSION.str();
            info << " (";
            if (strcmp(wcrev, "unknown")) info << wcrev << " / ";
            info << getosstring() << ")";
            info << "  Sauerbraten revision: " << SAUERSVNREV;
            info << "  Compiler: " << compiler[0] << " " << compiler[1];

            string enetversion;

#define ENET_1_3_8 66312
#if ENET_VERSION_CREATE(ENET_VERSION_MAJOR, ENET_VERSION_MINOR, ENET_VERSION_PATCH) >= ENET_1_3_8
            ENetVersion ev = enet_linked_version();

            formatstring(enetversion)("%d.%d.%d",
                         ENET_VERSION_GET_MAJOR(ev),
                         ENET_VERSION_GET_MINOR(ev),
                         ENET_VERSION_GET_PATCH(ev));
#else
            copystring(enetversion, "unknown");
#endif //>= ENET_1_3_8
#undef ENET_1_3_8

            libinfo << "libs: "
                    << "enet version: (" << enetversion << ")  "
                    << "curl version: (" << curl_version() << ")  "
                    << "geoip version: (" << GeoIP_lib_version() << ")  "
                    << "zlib version: (" << zlibVersion() << ")  "
                    << "openssl version: (" << SSLeay_version(SSLEAY_VERSION) << ")";
        }

        if (*get)
        {
            static string tmp = {0};

            if (!*tmp)
            {
                strtool buf(tmp, sizeof(tmp));
                buf = info; buf += '\n'; buf += libinfo;
            }

            result(tmp);
            return;
        }

        conoutf("%s", info.str());
        conoutf("%s", libinfo.str());
    }
    COMMAND(wcinfo, "i");

    //bouncer
    MODSVARP(bouncerkey, "");
    MODSVARP(bouncerpass, "");
    MODSVARP(bouncerhost, "");
    MODVARP(bouncerport, 0, 0, 0xFFFF);

    ENetAddress bouncerconnecthroughaddress;
    char *bouncerserverpass = NULL;

    // host of the server which we are connected to through the bouncer
    ENetAddress bouncerserverhost = {0};
 
    // bouncer extinfo relay address
    ENetAddress bouncerextinfohost = {0};

    bool isbouncerhost = false;
    bool havebouncerinfopacket = false;

    void unsetbouncervars()
    {
        zeroaddress(bouncerconnecthroughaddress);
        DELETEA(bouncerserverpass);

        zeroaddress(bouncerserverhost);
        zeroaddress(bouncerextinfohost);

        isbouncerhost = false;
        havebouncerinfopacket = false;
    }

    void sendbouncerconnectthroughpacket()
    {
        if (!bouncerconnecthroughaddress.host)
            return;

        ENetAddress addr;

        enet_address_set_host(&addr, bouncerhost);
        addr.port = bouncerport;

        if (!addressequal(addr, curpeer->address))
            return;

        packetbuf p(MAXTRANS, ENET_PACKET_FLAG_RELIABLE);

        putint(p, N_BOUNCER_CONNECT_THROUGH);
        putint(p, (int)bouncerconnecthroughaddress.host);
        putint(p, (int)bouncerconnecthroughaddress.port);

        uint flags = 0;

        //
        // 0x01: try to attach to existing client
        // 0x02: have pass
        // 0x04: persist
        // 0x08: take over name from *this* client
        // 0x10: no auto spec
        //

        flags |= 0x01; // try to attach to existing client

        if (bouncerserverpass)
            flags |= 0x02; // have server password

                         // no auto spec, so client spawns directly
         flags |= 0x10;  // this flag is only active when "tryattach"
                         // isn't set, or when there is no client to attach

        putint(p, (int)flags);

        if (bouncerserverpass)
            sendstring(bouncerserverpass, p);

        sendclientpacket(p.finalize(), 2);
    }

    void bouncerconnect(const char *servername, int *serverport, const char *serverpassword)
    {
        if (!bouncerhost[0])
        {
            erroroutf_r("set bouncerhost and bouncerport first");
            return;
        }

        if (servername[0])
        {
            enet_address_set_host(&bouncerconnecthroughaddress, servername);
            bouncerconnecthroughaddress.port = (*serverport)&0xFFFF;

            if (!bouncerconnecthroughaddress.port)
                bouncerconnecthroughaddress.port = server::serverport();

            if (*serverpassword)
                bouncerserverpass = newstring(serverpassword);

            if (havebouncerinfopacket)
            {
                ENetAddress addr;
                enet_address_set_host(&addr, bouncerhost);
                addr.port = bouncerport;

                if (addressequal(addr, curpeer->address))
                {
                    // already connected to bouncer

                    sendbouncerconnectthroughpacket();
                    return;
                }
            }
        }

        disconnect();
        connectserv(bouncerhost, bouncerport, bouncerpass);
        if (connpeer) isbouncerhost = true;
    }
    COMMAND(bouncerconnect, "sis");

    //network

    bool isipaddress(strtool &name)
    {
        strtool *strs = NULL;
        bool rv = false;
        size_t n;
        if ((n = name.split(".", &strs)) != 4) goto cleanup;
        loopi(4)
        {
            strtool &str = strs[i];
            if (str.empty() || str.length() > 3) goto cleanup;
            if (str[0] == '-' || str[0] == '+') goto cleanup;
            if (!str.isnumber()) goto cleanup;
            int num = str.tonumber<int>(-1, INT_MIN, INT_MAX);
            if (num < 0 || num > 0xFF) goto cleanup;
        }
        rv = true;
        cleanup:;
        delete[] strs;
        return rv;
    }

    //events

    void startup()
    {
        setnanosecondsbase(getnanoseconds());

        // dummy to prevent linking against wrong enet
        enet_do_not_link_against_wrong_lib(sizeof(realBandwidthStats));

        mainthreadid = SDL_ThreadID();
        threadmutex = SDL_CreateMutex();

        if (!threadmutex)
            fatal("failed to create mutex");

        crypto::init();
        geoip::startup();
        extinfo::startup();
        demorecorder::startup();
        searchdemo::startup();
        ipignore::startup();
        http::startup();
        event::startup();

        //run("startup"); // moved to main.cpp
    }

    void shutdown()
    {
        event::run(event::SHUTDOWN);

        extinfo::shutdown();
        demorecorder::shutdown();
        searchdemo::shutdown();
        ipignore::shutdown();
        http::shutdown(); // http needs to be shutdown before events, there might be event callbacks left
        event::shutdown();
        geoip::shutdown();
        crypto::deinit();

        Destroy_SDL_Mutex(threadmutex);
    }

    void slice()
    {
        static int last = 0;

        if (totalmillis-last < 10)
            return;

        {
            SDL_Mutex_Locker m(threadmutex);

            printthreadmessages();
            executethreadscripts();
        }

        plugin::slice();
        extinfo::slice();
        searchdemo::slice();
        gamemod::followactioncn();
        runtimerfunctions();
        calcclientbandwidthstats();
        http::slice();

        last = totalmillis;
    }

    void loadscripthook(const char *filename)
    {
        event::scriptfileloading(filename);
    }

    void scriptloadedhook(const char *filename)
    {
        event::scriptfileloaded(filename);
    }

    int uncompress(const void *data, size_t datalen, void **buf_out, size_t& len_out, size_t maxlen)
    {
        if (datalen < sizeof(uint))
            return UNCOMPRESS_ERROR_INVALID_BUFFER;

        len_out = *(uint*)((uchar*)data+datalen-sizeof(uint));
        lilswap(&len_out);

        if (len_out > maxlen)
            return UNCOMPRESS_ERROR_OUTPUT_SIZE_TOO_BIG;

        void *ubuf = malloc(len_out);

        if (!ubuf)
            return UNCOMPRESS_ERROR_FAILED_TO_ALLOCATE_BUFFER;

        z_stream zstream = {0};

        zstream.total_in = zstream.avail_in = datalen;
        zstream.total_out = zstream.avail_out = len_out;
        zstream.next_in = (Bytef*)data;
        zstream.next_out = (Bytef*)ubuf;

        if (inflateInit2(&zstream, 16+MAX_WBITS) != Z_OK)
        {
            inflateEnd(&zstream);
            free(ubuf);
            return UNCOMPRESS_ERROR_ZLIB;
        }

        if (inflate(&zstream, Z_FINISH) != Z_STREAM_END)
        {
            inflateEnd(&zstream);
            free(ubuf);
            return UNCOMPRESS_ERROR_OTHER;
        }

        len_out = zstream.total_out;
        *buf_out = ubuf;

        inflateEnd(&zstream);

        return UNCOMPRESS_OK;
    }

    bool getcommandpath(const char *command, strtool &path)
    {
        char *PATH = getenv("PATH");
        assert(PATH && "this shouldn't happen");

        struct stat st;
        strtool file;

        const char *p = PATH;

        do
        {
            if (*p == ':')
                p++;

            p += path.copyuntil(p, ":");

            path << "/" << command;

            if (!stat(path.str(), &st))
            {
#ifndef _WIN32
                if (access(path.str(), F_OK|X_OK))
                    continue;
#endif //!_WIN32
                break;
            }

            path.clear();
            p++;
        } while (*p);

        return path;
    }

    size_t runcommand(const char *command, char *buf, size_t len)
    {
        #define RETURN(v) \
        do { \
            if (p) pclose(p); \
            return (v); \
        } while (0)

        if (!len)
            return RUNCOMAND_ERROR;

        FILE *p;
        size_t outputlen;

        if (!(p = popen(command, "r")))
            RETURN(RUNCOMAND_ERROR);

        if (!(outputlen = fread(buf, sizeof(char), len-1, p)))
            RETURN(RUNCOMAND_ERROR);

        buf[outputlen] = '\0';

        assert(outputlen != RUNCOMAND_ERROR);

        RETURN(outputlen);
        #undef RETURN
    }

    struct threadmemory
    {
        int type;
        void *p;
        Uint32 threadid;

        threadmemory(int type, void *p) : type(type), p(p), threadid(SDL_ThreadID()) { }
    };

    struct threadstorage
    {
        SDL_mutex *m;
        vector<threadmemory> allocs;

        threadstorage() : m(SDL_CreateMutex())
        {
            if (!m)
                abort();
        }

        ~threadstorage()
        {
            lock();

            if (!allocs.empty())
                abort();

            unlock();
            SDL_DestroyMutex(m);
        }

        void lock() { SDL_mutexP(m); }
        void unlock() { SDL_mutexV(m); }

        void* allocate(int type, size_t size, bool checkptr)
        {
            void *p = malloc(size);

            if (!p)
            {
                if (checkptr)
                    abort();

                return NULL;
            }

            lock();
            allocs.add(threadmemory(type, p));
            unlock();

            return p;
        }

        void deallocate(int type)
        {
            Uint32 threadid = SDL_ThreadID();

            lock();
            loopvrev(allocs)
            {
                threadmemory &tm = allocs[i];
                if (tm.threadid == threadid && tm.type == type)
                {
                    free(tm.p);
                    allocs.remove(i);
                }
            }
            unlock();
        }
    } threadstorage;

    void *allocatethreadstorage(int type, size_t size, bool checkptr, void *mainbuf, bool copy)
    {
        if (mainbuf && ismainthread())
            return mainbuf;

        void *p = threadstorage.allocate(type, size, checkptr);

        if (copy && p && mainbuf)
            memcpy(p, mainbuf, size);

        return p;
    }

    void deallocatethreadstorage(int type)
    {
        threadstorage.deallocate(type);
    }

    bool isnumeric(const char *str)
    {
        if (!*str) return false;
        if (*str == '-') str++;
        for(; *str; str++)
            if (*str > '9' || *str < '0') return false;
        return true;
    }

    bool binfile::putstring(const char *str, uint len)
    {
        if (!len)
            len = strlen(str);

        if (!putobj(&len))
            return false;

        for(; *str; str++)
        {
            if (!putobj(&*str))
                return false;
        }

        return true;
    }

    char *binfile::getstring(uint *length)
    {
        uint len = 0;

        if (!length)
            length = &len;

        if (!getobj(length))
            return NULL;

        static const uint MAXLEN = 1024*1024;

        if (*length >= MAXLEN)
            return NULL;

        char *buf = new char[(*length)+1];

        loopi(len)
        {
            if (!getobj(&(buf[i])))
            {
                delete[] buf;
                return NULL;
            }
        }

        buf[*length] = '\0';

        return buf;
    }
#endif //STANDALONE
}
