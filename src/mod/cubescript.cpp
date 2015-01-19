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

// cubescript extensions

#include "game.h"

namespace mod
{
    const char *escapecubescriptstring(const char *str, strtool &v, bool quotes)
    {
        if (quotes)
            v.add('"');

        // same as escapestring() from command.cpp but thread safe
        for(const char *s = str; *s; s++) switch(*s)
        {
            case '\n': v.append("^n", 2); break;
            case '\t': v.append("^t", 2); break;
            case '\f': v.append("^f", 2); break;
            case '"': v.append("^\"", 2); break;
            case '^': v.append("^^", 2); break;
            default: v.add(*s); break;
        }

        if (quotes)
            v.add('"');

        return v.str();
    }

    namespace http
    {
        struct cbarg
        {
            uint scriptcallback;
            uint scriptcallbackid;

            cbarg(uint scriptcallback, uint scriptcallbackid) :
                  scriptcallback(scriptcallback), scriptcallbackid(scriptcallbackid) {}
        };

        void freedata(void *data)
        {
            delete (cbarg*)data;
        }

        void callback(uint requestid, cbarg *arg, bool ok, int responsecode, const char *data, size_t datalen)
        {
            event::run(arg->scriptcallback, "ddsd", ok ? 1 : 0, responsecode, data ? data : "", (int)datalen);
            event::uninstall(arg->scriptcallbackid);
        }

        void request_(const char *request, const char *callbackargs, const char *callbackcode, const char *referer, const char *useragent)
        {
            if (!*request)
            {
                intret(0);
                return;
            }

            uint scriptcallback = 0;
            uint scriptcallbackid = 0;

            if (*callbackargs && *callbackcode)
            {
                uint callbackid;
                scriptcallbackid = event::install(event::INTERNAL_CALLBACK, callbackargs, callbackcode, &callbackid);
                scriptcallback = callbackid<<16|event::INTERNAL_CALLBACK;
            }

            cbarg *arg = scriptcallback ? new cbarg(scriptcallback, scriptcallbackid) : NULL;

            auto *httpreq = new http::request_t;

            httpreq->request = request;
            if (*referer) httpreq->referer = referer;
            if (*useragent) httpreq->useragent = useragent;

            if (scriptcallback)
            {
                httpreq->callbackdata = arg;
                httpreq->callback = (callback_t)callback;
                httpreq->freedatacallback = freedata;
            }

            uint reqid = http::request(httpreq);

            if (reqid == UNABLE_TO_PROCESS_REQUEST_NOW)
            {
                if (scriptcallbackid)
                    event::uninstall(scriptcallbackid);
            }

            intret((int)reqid);
        }
        COMMANDN(httprequest, request_, "ssssss");

        void uninstall(int *id)
        {
            intret(uninstallrequest((uint)*id, true) ? 1 : 0);
        }
        COMMANDN(delhttprequest, uninstall, "i");

        void responsecodeok(int *responsecode)
        {
            int retval;

            switch (*responsecode)
            {
            case 200:
                retval = 1;
                break;

            case QUERY_ABORTED:
            case INVALID_RESPONSE_CODE:
            default:
                retval = 0;
            }

            intret(retval);
        }
        COMMANDN(httpresponsecodeok, responsecodeok, "i");
    }

    namespace scriptvector
    {
        struct scriptvec
        {
            char *name;

            scriptvec(const char *name_in) : name(newstring(name_in)) { }
            ~scriptvec() { delete[] name; }

            struct value
            {
                char *index;
                char *valbuf;
                size_t buflen;

                void updateval(const char *valstr)
                {
                    size_t len = strlen(valstr)+1;
                    if (!valbuf || len > buflen)
                    {
                        if(valbuf) delete[] valbuf;
                        valbuf = new char[len];
                        buflen = len;
                    }
                    memcpy(valbuf, valstr, len);
                }

                value(const char *index_in) : index(newstring(index_in)), valbuf(NULL), buflen(0) { }
                ~value() { delete[] index; if(valbuf) delete[] valbuf; }
            };

            hashtable<const char *, value *> values;

            bool deleteindex(const char *index)
            {
                value *v = values.find(index, NULL);
                if(!v) return false;
                values.remove(index);
                delete v;
                return true;
            }

            void deletevalues()
            {
                enumerate(values, value *, v, delete v);
                values.clear();
            }
        };

        static hashtable<const char *, scriptvec *> scriptvecs;

        void scriptvector(const char *name, const char *index, const char *value, int *nowarning)
        {
            scriptvec *sv = scriptvecs.find(name, NULL);

            if(!sv)
            {
                if(*index || *value)
                {
                    if(nowarning && !*nowarning) conoutf(CON_ERROR, "\fs\f3error:\fr script vector: accessing invalid script vector '%s' (not created)", name);
                    intret(0);
                    return;
                }
                sv = new scriptvec(name);
                scriptvecs.access(sv->name, sv);
                intret(1);
                return;
            }

            if(!*index)
            {
                sv->deletevalues();
                intret(1);
                return;
            }

            scriptvec::value *v = sv->values.find(index, NULL);

            if(!v)
            {
                if(!*value)
                {
                    if(nowarning && !*nowarning) conoutf(CON_ERROR, "\fs\f3error:\fr script vector '%s': accessing invalid index '%s'", sv->name, index);
                    intret(0);
                    return;
                }
                v = new scriptvec::value(index);
                sv->values.access(v->index, v);
            }

            if(!*value)
            {
                result(v->valbuf);
                return;
            }

            v->updateval(value);
            intret(1);
        }
        COMMAND(scriptvector, "sssi");

        void delscriptvectorindex(const char *name, const char *index)
        {
            scriptvec *sv = scriptvecs.find(name, NULL);
            intret(sv && sv->deleteindex(index) ? 1 : 0);
        }
        COMMAND(delscriptvectorindex, "ss");

        void delscriptvector(const char *name)
        {
            scriptvec *sv = scriptvecs.find(name, NULL);
            if(!sv)
            {
                intret(0);
                return;

            }
            scriptvecs.remove(name);
            delete sv;
            intret(1);
        }
        COMMAND(delscriptvector, "s");

        void loopscriptvector(const char *name, ident *index, ident *value, uint *body, int *start, int *end)
        {
            if(index->type!=ID_ALIAS || value->type!=ID_ALIAS) return;
            scriptvec *sv = scriptvecs.find(name, NULL);
            if(!sv)
            {
                intret(0);
                return;
            }
            if(!sv->values.numelems) return;
            identstack stack[2];
            int j = 0; bool pop = false;
            enumerate(sv->values, scriptvec::value *, v,
            {
                if(v && ((!*start && !*end) || (++j >= *start && j <= *end)))
                {
                    tagval t[2];
                    t[0].setstr(newstring(v->index));
                    t[1].setstr(newstring(v->valbuf));
                    pusharg(*index, t[0], stack[0]);
                    pusharg(*value, t[1], stack[1]);
                    index->flags &= ~IDF_UNKNOWN;
                    value->flags &= ~IDF_UNKNOWN;
                    execute(body);
                    pop = true;
                }
            });
            if(pop)
            {
                poparg(*index);
                poparg(*value);
            }
        }
        COMMAND(loopscriptvector, "srreii");

        void scriptvectorlength(const char *name)
        {
            scriptvec *sv = scriptvecs.find(name, NULL);
            if(!sv)
            {
                intret(0);
                return;
            }
            intret(sv->values.numelems);
        }
        COMMAND(scriptvectorlength, "s");

        void writescriptvector(const char *name, const char *file)
        {
            scriptvec *sv = scriptvecs.find(name, NULL);
            if(!sv)
            {
                intret(0);
                return;
            }
            defformatstring(filename)("%s.scriptvec", file);
            binfile bf(filename);
            if (!bf.ok()) return;
            bf.putstring(name);
            enumerate(sv->values, scriptvec::value *, v,
            {
                if(v)
                {
                    bf.putstring(v->index);
                    bf.putstring(v->valbuf);
                }
            });
            intret(1);
        }
        COMMAND(writescriptvector, "ss");

        void loadscriptvector(const char *file)
        {
            defformatstring(filename)("%s.scriptvec", file);
            binfile bf(filename, "rb");
            char *vecname = NULL;
            if (!bf.ok())
            {
                error:;
                intret(0);
                return;
            }
            vecname = bf.getstring();
            if (!vecname) goto error;
            scriptvector(vecname, "", "", NULL);
            while(true)
            {
                char *var = bf.getstring();
                char *value = bf.getstring();
                if (!var || !value)
                {
                    delete[] var;
                    delete[] value;
                    if (var == value) goto ok;
                    delete[] vecname;
                    goto error;
                }
                scriptvector(vecname, var, value, NULL);
                delete[] var;
                delete[] value;
            }
            ok:;
            delete[] vecname;
            intret(1);
        }
        COMMAND(loadscriptvector, "s");

    } // scriptvector

    namespace network
    {
        struct resolve
        {
            uint32_t ip;
            strtool ipaddr;
            strtool hostname;
            strtool message;
            uint scriptcallback;
            uint scriptcallbackid;

            bool isresolved()
            {
                return hostname && hostname != ipaddrstr();
            }

            strtool &ipaddrstr()
            {
                if (!ipaddr)
                {
                    string tmp;
                    ENetAddress addr = { ip, 0 };
                    if (enet_address_get_host_ip(&addr, tmp, sizeof(tmp)) < 0)
                        tmp[0] = '\0';
                    ipaddr = tmp;
                }
                return ipaddr;
            }

            resolve(uint32_t ip,strtool &message) :
                ip(ip), message(message),
                scriptcallback(), scriptcallbackid() {}

            resolve() {}
        };

        const int queuesize = 64;
        const int maxthreads = 12;

        static queue<resolve*, queuesize> toresolve;
        static queue<resolve*, queuesize> resolved;

        static uint timerid = 0;

        static sdlmutex *mutex = NULL;
        static sdlcondition *cond = NULL;
        static vector<sdlthread*> threads;

        static int available = 0;
        static bool exitthread = false;

        static int resolverthread(void *)
        {
            mutex->lock();

            do
            {
                if (exitthread)
                {
                    available--;
                    mutex->unlock();
                    return 0;
                }

                if (!toresolve.empty())
                {
                    resolve *r = toresolve.remove();

                    mutex->unlock();

                    string hostname;
                    ENetAddress addr = { r->ip, 0 };

                    if (enet_address_get_host(&addr, hostname, sizeof(hostname)) < 0)
                        hostname[0] = '\0';

                    r->hostname = hostname;

                    mutex->lock();
                    resolved.add() = r;
                }

                if (toresolve.empty() && !exitthread)
                {
                    available++;
                    cond->wait(*mutex);
                }
            } while (true);
        }

        static void check()
        {
            resolve *r;

            do
            {
                r = NULL;

                mutex->lock();
                if (!resolved.empty()) r = resolved.remove();
                mutex->unlock();

                if (!r)
                    break;

                if (r->scriptcallbackid)
                {
                    event::run(r->scriptcallback, "ss", r->message.str(), r->hostname.str());
                    event::uninstall(r->scriptcallbackid);
                }
                else
                {
                    conoutf("\fs\f1%s\fr -> (ip: \fs\f1%s\fr  hostname: \fs\f1%s\fr)",
                            r->message.str(), r->ipaddrstr().str(),
                            r->isresolved() ? r->hostname.str() : "unknown");
                }

                delete r;
            } while (true);
        }

        static void shutdown()
        {
            removetimerfunction(timerid);

            {
                SDL_Mutex_Locker m(*mutex);
                exitthread = true;
            }

            cond->signalall();

            // not rendered anymore but shown in the (terminal) console
            conoutf("waiting for resolver threads...");

            loopv(threads)
                delete threads[i];

            delete cond;
            delete mutex;

            threads.setsize(0);
            cond = NULL;
            mutex = NULL;
        }

        static void resolveip(const char *ip, const char *callbackargs, const char *callbackcode)
        {
            if (timerid && threads.empty())
            {
                intret(0);
                return;
            }
            if (!*ip)
            {
                erroroutf_r("no ip address given");
                intret(0);
                return;
            }

            strtool message;
            ipmask iaddr;
            uint32_t ipaddr = 0;

#ifdef ENABLE_IPS
            char *end = NULL;
            int cn = strtol(ip, &end, 10);

            if (end && *end != '.')
            {
                if (cn < MAXCLIENTS*2 && cn >= 0)
                {
                    fpsent *d = game::getclient(cn);

                    if (!d)
                    {
                        erroroutf_r("invalid cn");
                        intret(0);
                        return;
                    }

                    message.fmt("%s (%d)", d->name, d->clientnum);

                    if (!d->extinfo)
                    {
                        erroroutf_r("no extinfo information available for player %s", message.str());
                        intret(0);
                        return;
                    }

                    auto ip = d->extinfo->ip;

                    if (ip.ui32 == uint32_t(-1))
                    {
                        erroroutf_r("ip address of player %s is not known", message.str());
                        intret(0);
                        return;
                    }

                    ip.ia[3] = 128;
                    ipaddr = ip.ui32;
                }
                else if (cn == -1)
                {
                    loopv(game::clients)
                    {
                        fpsent *d = game::clients[i];

                        if (!d)
                            continue;

                        defformatstring(cn)("%d", d->clientnum);
                        resolveip(cn, "", "");
                    }

                    return;
                }

                goto haveip;
            }
#endif //ENABLE_IPS

            message = ip;
            iaddr.parse(ip);
            ipaddr = iaddr.ip;

#ifdef ENABLE_IPS
            haveip:;
#endif //ENABLE_IPS

            if (threads.empty())
            {
                if (!event::install(event::SHUTDOWN, shutdown))
                {
                    erroroutf_r("can't install shutdown event");
                    intret(0);
                    return;
                }

                mutex = new sdlmutex;
                cond = new sdlcondition;

                addtimerfunction(0, false, check);
            }

            mutex->lock();

            if ((available <= 0 || toresolve.length() >= available) && threads.length() < maxthreads)
            {
                mutex->unlock();

                threads.add(new sdlthread(resolverthread));

                while (threads.length() == 1)
                {
                    SDL_Mutex_Locker ml(*mutex);
                    if (available) break;
                    SDL_Delay(1);
                }
            }
            else
            {
                mutex->unlock();
            }

            resolve *r = new resolve(ipaddr, message);

            if (*callbackargs && *callbackcode)
            {
                uint callbackid;
                r->scriptcallbackid = event::install(event::INTERNAL_CALLBACK, callbackargs, callbackcode, &callbackid);
                r->scriptcallback = callbackid<<16|event::INTERNAL_CALLBACK;
            }

            {
                SDL_Mutex_Locker ml(*mutex);

                if (toresolve.full())
                {
                    if (r->scriptcallbackid)
                        event::uninstall(r->scriptcallbackid);
                    delete r;
                    erroroutf_r("resolver queue is full (in)!");
                    intret(0);
                    return;
                }

                toresolve.add() = r;
            }

            cond->signalone();
            intret(1);
        }
        COMMAND(resolveip, "sss");

        namespace autoresolveip 
        {
            static vector<uint> resolvedcns;

            static int playerdisconnect(const char *argfmt, va_list args)
            {
                assert(*argfmt == 'd');
                resolvedcns.removeobj((uint)va_arg(args, int));
                return 0;
            }

            static void clearresolvedcns()
            {
                resolvedcns.setsize(0);
            }

            static int extinfoupdate(const char *argfmt, va_list args)
            {
                assert(*argfmt == 'u');
                uint cn = va_arg(args, uint);

                if (resolvedcns.find(cn) >= 0)
                  return 0;

                if (game::getclient(cn)->extinfo->ip.ui32 == uint32_t(-1))
                  return 0;

                defformatstring(cnstr)("%u", cn);
                resolveip(cnstr, "", "");
                resolvedcns.add(cn);
                return 0;
            }

            void autoresolveipchanged()
            {
                static vector<uint> eventids(4);
                extern int autoresolveip;

                if (autoresolveip)
                {
                    if (eventids.empty())
                    {
                        eventids.add() = event::xinstall(event::EXTINFO_UPDATE, extinfoupdate);
                        eventids.add() = event::xinstall(event::PLAYER_DISCONNECT, playerdisconnect);
                        eventids.add() = event::xinstall(event::CONNECT, clearresolvedcns);
                        eventids.add() = event::xinstall(event::DISCONNECT, clearresolvedcns);
                    }
                }
                else if (!eventids.empty())
                {
                    loopv(eventids) event::uninstall(eventids[i]);
                    eventids.setsize(0);
                }
            }

            MODVARFP(autoresolveip, 0, 0, 1, autoresolveipchanged());
        }

        ICOMMAND(isipaddress, "s", (const char *s), intret(isipaddress(s)));
    }

    namespace parsevarnames
    {
        /*
         * Functions to inspect script body for
         * used variable names.
         */

        static inline bool iswhitespace(const char c)
        {
            switch (c)
            {
                case ' ': case '\r': case '\n':
                case '\t': case '(': case ')': case ';':
                case '[': case ']':
                    return true;
            }

            return false;
        }

        static inline const char* parsevar(const char *&code, char *buf)
        {
            int len = 0;
            while (!iswhitespace(*++code) && *code)
                len++;

            len = min(len, (int)sizeof(string)-1);
            memcpy(buf, code-len, len);
            buf[len] = 0;

            if (!*buf)
                return NULL;

            return buf;
        }

        bool varexists(vector<char*> &vars, const char *var)
        {
            loopv(vars)
            {
                if (!strcmp(vars[i], var))
                    return true;
            }

            return false;
        }

        static inline void addvar(vector<char*> &vars, const char *var)
        {
            if (varexists(vars, var))
                return;

            vars.add(newstring(var));
        }

        static inline char* skipats(char *str)
        {
            char *c = str;

            while (*c == '@' && *c)
                c++;

            return c;
        }

        static bool parsestring(char **str, char *error, size_t size)
        {
            strtool errstr(error, size);
            char *p = *str;

            if (!*p)
                return false;

            if (*p != '"')
            {
                int i = strlen(p)-1;

                if (p[i] == '"')
                {
                    p[i] = 0;
                    errstr.fmt("variable around '%s' not properly escaped (don't use spaces)", p);
                    return false;
                }

                return true;
            }

            p++;
            p[::parsestring(p)-p] = 0;

            if (!*p)
            {
                errstr.copy("empty variable name detected");
                return false;
            }

            *str = p;
            return true;
        }

        bool parsevarnames(vector<char*> &vars, const char *code, char *error, size_t size)
        {
            const char *c = code;
            bool isstring = false;

            while (*c)
            {
                switch (*c)
                {
                    case '^':
                    {
                        if (isstring && !*++c)
                        {
                            copystring(error, "invalid escape sequence detected.", size);
                            goto error;
                        }

                        break;
                    }

                    case '"':
                    {
                        isstring = !isstring;
                        break;
                    }

                    case '=':
                    {
                        if (isstring)
                            break;

                        const char *p = c;

                        if (p[1] != ' ' && p[1] != '\t')
                            break;

                        if (p == code || (p[-1] != ' ' && p[-1] != '\t'))
                            break;

                        /* var<-- skip this whitespaces -->= xy */

                        while (p > code && iswhitespace(*--p));

                        /* <-- loop until we are getting here -->var = xy */

                        while (p > code && !iswhitespace(*--p));

                        string var;
                        if (!parsevar(p, var))
                            break;

                        char *v = var;
                        if (!parsestring(&v, error, size))
                            goto error;

                        if (*v == '@')
                            v = skipats(v);

                        addvar(vars, v);

                        break;
                    }

                    case '$': case '@':
                    {
                        if (isstring)
                            break;

                        if (*c == '@')
                        {
                            c = skipats((char*)c);
                            c--;
                        }

                        string var;
                        if (!parsevar(c, var))
                            break;

                        addvar(vars, var);

                        break;
                    }
                }

                c++;
            }

            return true;

            error:
            vars.deletearrays();
            return false;
        }

    } // parsevars
}
