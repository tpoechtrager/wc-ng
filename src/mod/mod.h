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

// included in cube.h
// generic mod header

#ifndef __MOD_H__
#define __MOD_H__

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define sizeofarray(a) (sizeof(a)/sizeof(*a))
#define STRLEN(c) (sizeof(c)-1)

#define declfun(fun, name) decltype(&fun) name = fun

#include "compiler.h"
#include "strtool.h"
#include "crypto.h"

// TODO: move this out
extern ENetPeer *curpeer;
extern ENetPeer *connpeer;

namespace mod
{
#ifndef STANDALONE
    //
    // returns true if the calling thread is the main thread.
    //
#ifndef PLUGIN
    bool ismainthread();
#else
    static inline bool ismainthread() { return true; }
#endif //!PLUGIN

    //
    // thread safe variant of conoutf(),
    // if this function is called from the main thread,
    // then the normal conoutf() is called.
    //
    void addthreadmessage(strtool &msg, int con = CON_INFO);
    void conoutf_r(int con, const char *fmt, ...) PRINTFARGS(2, 3);
    void conoutf_r(const char *fmt, ...) PRINTFARGS(1, 2);

    //
    // thread safe erroroutf().
    //
    void erroroutf_r(const char *fmt, ...) PRINTFARGS(1, 2);

    //
    // thread safe variant of execute().
    //
    enum
    {
        EXECUTE_R_EXECUTED_IN_MAIN = 0,
        EXECUTE_R_REMOVE_SUCCESS,
        EXECUTE_R_INVALID_ID
    };
    uint execute_r(const char *code, size_t codelen = 0, bool disown = false,
                   void (*callback)(int, void *) = NULL,
                   void *callbackarg = NULL, int *mainresult = NULL);

    //
    // remove waiting script from the queue
    //
    // returns EXECUTE_R_REMOVE_SUCCESS on sucess,
    // otherwise EXECUTE_R_INVALID_ID is returned.
    //
    int remove_execute_r(uint id);

    // misc
    template<class N>
    static inline const char *plural(N n)
    {
        if (n != N(1)) return "s";
        return "";
    }

    static inline bool is64bitpointer()
    {
        return sizeof(void*) == 8;
    }

    static inline const char *getosstring()
    {
#ifdef WIN32
         return is64bitpointer() ? "win64" : "win32";
#else
#ifdef __linux__
        return is64bitpointer() ? "linux64" : "linux32";
#else
#ifdef __APPLE__
        return is64bitpointer() ? "osx64" : "osx32";
#else
#ifdef __FreeBSD__
        return is64bitpointer() ? "freebsd64" : "freebsd32";
#else
        return is64bitpointer() ? "other64" : "other32";
#endif //BSD
#endif //__APPLE__
#endif //__linux__
#endif //WIN32
    }

    enum
    {
        UNCOMPRESS_OK,
        UNCOMPRESS_ERROR_OUTPUT_SIZE_TOO_BIG,
        UNCOMPRESS_ERROR_FAILED_TO_ALLOCATE_BUFFER,
        UNCOMPRESS_ERROR_INVALID_BUFFER,
        UNCOMPRESS_ERROR_ZLIB,
        UNCOMPRESS_ERROR_OTHER
    };
    int uncompress(const void *data, size_t datalen, void **buf_out, size_t& len_out, size_t maxlen = -1);
    static const size_t RUNCOMAND_ERROR = (size_t)-1;
    bool getcommandpath(const char *command, strtool &path);
    size_t runcommand(const char *command, char *buf, size_t len);
#endif //!STANDALONE
#if !defined(PLUGIN) && !defined(STANDALONE)
    void* allocatethreadstorage(int type, size_t size, bool checkptr = false, void *mainbuf = NULL, bool copy = false);
#else
    static inline void* allocatethreadstorage(int type, size_t size, bool checkptr, void *mainbuf, bool copy = false) { return mainbuf; }
#endif //!PLUGIN && !STANDALONE
#ifndef STANDALONE
    void deallocatethreadstorage(int type);
    bool isnumeric(const char *str);
    void dumppacketbuf(ucharbuf &p);
    int distinctlistfiles(const char *dir, const char *ext, vector<char*>& files);
    void wccheckversion(bool userrequest = false, int n = 0);
    void wcautocheckversion();

    // timers
    void setnanosecondsbase(ullong baseval);
    ullong getnanoseconds();
    inline ullong getmicroseconds() { return getnanoseconds()/1000; }
    inline ullong getmilliseconds() { return getmicroseconds()/1000; }
    uint getsaltedmicroseconds(bool newsalt = false, uint saltedmicroseconds = -1, int *salt = NULL,
                               void (*seed)(uint) = seedMT, uint (*rand)() = randomMT);

    // timer functions
    typedef void (*timerfunctype1)();
    typedef void (*timerfunctype2)(void *cbdata, uint timerid);

    template<class T> struct timerfunctype;
    template<> struct timerfunctype<timerfunctype1> { static const int type = 1; };
    template<> struct timerfunctype<timerfunctype2> { static const int type = 2; };

    uint addtimerfunction(int interval, bool once, void *callback, int type, void *cbdata);
    bool removetimerfunction(uint id, void (*freecallback)(void *cbdata) = NULL);

    template<class T>
    static uint addtimerfunction(int interval, bool once, T callback, void *cbdata = NULL)
    {
        // static_assert(sizeof(void*) == sizeof(T), "");
        return addtimerfunction(interval, once, (void*)callback, timerfunctype<T>::type, cbdata);
    }

    // cubescript extensions
    const char* escapecubescriptstring(const char *str, strtool &v, bool quotes = true);
    namespace parsevarnames
    {
        bool parsevarnames(vector<char*> &vars, const char *code, char *error, size_t size);
        bool varexists(vector<char*> &vars, const char *var);
    }

    // events
    void startup();
    void shutdown();
    void slice();
    void loadscripthook(const char *filename);
    void scriptloadedhook(const char *filename);

    // bouncer
    enum
    {
        N_BOUNCER_FIRST             = -1857832,
        N_BOUNCER_SERVERHOST        = N_BOUNCER_FIRST-1,
        N_BOUNCER_EXTINFOHOST       = N_BOUNCER_FIRST-2,
        N_BOUNCER_USINGRC4          = N_BOUNCER_FIRST-3, /* unused, kept for compatibility */
        N_BOUNCER_RC4KEY            = N_BOUNCER_FIRST-4, /* unused, kept for compatibility */
        N_BOUNCER_INFO              = N_BOUNCER_FIRST-5,
        N_BOUNCER_CONNECT_THROUGH   = N_BOUNCER_FIRST-6
    };

    extern char *bouncerkey;
    extern char *bouncerpass;
    extern char *bouncerhost;
    extern int bouncerport;
    extern ENetAddress bouncerconnecthroughaddress;
    extern char *bouncerserverpass;
    extern ENetAddress bouncerserverhost;
    extern ENetAddress bouncerextinfohost;
    extern bool isbouncerhost;
    extern bool havebouncerinfopacket;

    void unsetbouncervars();
    void sendbouncerconnectthroughpacket();

    // network
    static inline void zeroaddress(ENetAddress &address)
    {
        address.host = 0;
        address.port = 0;
    }
    static inline bool addressequal(const ENetAddress &a, const ENetAddress &b)
    {
        return (a.host == b.host && a.port == b.port);
    }
    static inline const char *resolve(enet_uint32 ip, string &resolvedip)
    {
        ENetAddress addr { ip, 0 };
        if (enet_address_get_host_ip(&addr, resolvedip, sizeof(resolvedip)) < 0)
            resolvedip[0] = '\0';
        return resolvedip;
    }
    bool isipaddress(strtool &name);
    static inline bool isipaddress(const char *name)
    {
        strtool tmp = name;
        return isipaddress(tmp);
    }

    class binfile
    {
    public:
        binfile(const char *file, const char *mode = "wb") : f(openfile(file, mode)) { }
        ~binfile() { delete f; }

        bool ok() const { return !!f; }
        stream::offset size() const { return f->size(); }

        template<class T>
        bool putobj(T *obj)
        {
            lilswap(obj, sizeof(*obj));
            return f->write(obj, sizeof(*obj)) == (int)sizeof(*obj);
        }

        template<class T>
        T* getobj(T *obj)
        {
            if (f->read(obj, sizeof(*obj)) != (int)sizeof(*obj))
                return NULL;

            lilswap(obj, sizeof(*obj));
            return obj;
        }

        bool putstring(const char *str, uint len = 0);
        char* getstring(uint *length = NULL);
    private:
        stream *f;
    };
#endif //STANDALONE
}

// TODO: move this out
namespace game
{
    // scoreboard
    bool isscoreboardshown();

    // cubescript func
    int getactioncn(bool cubescript = false);
}

template <class L, class R>
void assign(L &l, R &&r)
{
    l = (L)r;
}

class noncopyable
{
protected:
    noncopyable() {}
    ~noncopyable() {}
private:
    noncopyable (const noncopyable &);
    noncopyable &operator=(const noncopyable &);
};

template<class T>
class uniqueptr : noncopyable
{
    public:
        uniqueptr(T data, bool isarray = false) : data(data), isarray(isarray) {}
        ~uniqueptr()
        {
            if (isarray) delete[] data;
            else delete data;
        }

        T &operator*() { return *data; }
        T operator->() { return data; }
        operator T() { return data; }

    private:
        T data;
        const bool isarray;
};

#ifndef STANDALONE

class benchmark
{
public:
    bool active() { return running; }
    double gettime() { assert(!running); return time; }
    void reset() { running = false; }
    void start() { running = true; starttime = mod::getnanoseconds(); }
    void calc() { time = (mod::getnanoseconds()-starttime)/1000000.0; reset(); }

    benchmark(const char *desc_in, bool justinit = false)
    {
        copystring(desc, desc_in);
        running = false;
        time = 0.0;

        if (!justinit)
            start();
    }

    ~benchmark()
    {
        if (running)
        {
            calc();
            mod::conoutf_r("%s took: %.6f ms", desc, time);
        }
    }

private:
    string desc;
    bool running;
    ullong starttime;
    double time;
};

#define BENCHMARK(desc, code) do { benchmark bench(desc); code } while(0)

struct clientbwstats
{
    realBandwidthStats last;
    int millis;

    float curincomingkbs;
    float curoutgoingkbs;
    int curincomingpacketssec;
    int curoutgoingpacketssec;

    float incomingtotal;
    float outgoingtotal;

    clientbwstats() { memset(this, 0, sizeof(*this)); }
};

struct checkcalldepth
{
    bool overflow;
    int &calldepth;
    bool &warningshown;
    const char *func;
    int limit;

    void refresh()
    {
        if ((overflow = (++calldepth >= limit)))
        {
            if (!warningshown)
            {
                conoutf("\fs\f3warning:\fr call recursion limit reached\fs\f3(%s)\fr.", func);
                warningshown = true;
            }

            overflow = true;
        }
    }

    checkcalldepth(int max, int &cd, bool &cdw, const char *function) :
        calldepth(cd), warningshown(cdw), func(function), limit(max) { refresh(); }

    ~checkcalldepth() { if (!--calldepth) warningshown = false; }
};

#define CHECKCALLDEPTH(max, overflowinst)                  \
    static bool __calldepthwarningshown = false;           \
    static int __calldepth = 0;                            \
    checkcalldepth cd(max, __calldepth,                    \
        __calldepthwarningshown, __FUNCTION__);            \
    if (cd.overflow) do { overflowinst; } while(0)         \

#if defined(WIN32) || defined(__APPLE__)
// workaround for systems with case insensitive file systems
#define LIB_GEOIP_HEADER "../include/GeoIP/GeoIP.h"
#define MOD_GEOIP_HEADER "../mod/geoip.h"
#else
#define LIB_GEOIP_HEADER "GeoIP.h"
#define MOD_GEOIP_HEADER "geoip.h"
#endif //WIN32 || __APPLE__

#endif //STANDALONE

#include "ipbuf.h"
#include "wcversion.h"

#ifndef STANDALONE

#include "gamemod.h"
#include "http.h"
#include MOD_GEOIP_HEADER
#include "ipignore.h"
#include "extinfo.h"
#include "demorecorder.h"
#include "chat.h"
#include "events.h"
#include "thread.h"

#endif //STANDALONE
#endif //__MOD_H__
