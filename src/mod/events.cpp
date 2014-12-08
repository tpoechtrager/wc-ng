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

namespace mod {
namespace event
{
    struct warnmsg
    {
        const uint event;
        const char *const msg;
        warnmsg(uint event = INVALID, const char *msg = NULL) : event(event), msg(msg) {}
    };

    //
    // Events in the following table will emit a warning when
    // they are installed
    //

    static const warnmsg warn[] =
    {
        warnmsg(CONSOLE_INPUT, "may log your keyboard input"),
        warnmsg()
    };

    static const uint MAXEVENTS = 100000;
    static const int MAXEVENTARGS = 35;

    static uint eventcount = 0;
    static int forcecheck = 0;
    MODVARP(eventsystemnoduplicatewarning, 0, 0, 1);
    MODVARP(eventsystemnounusedcallargwarning, 0, 0, 1);

    static vector<uint> availableids;

    class event_t;
    typedef vector<event_t*> eventvector_t;

    class event_t
    {
    public:
        int gettype() { return type; }

        bool cmp(uint id_in) { return id == id_in; }
        bool namecmp(const char *name_in) { return !strcmp(getname(), name_in); }
        bool codecmp(const char *codestring_in) { return !strcmp(codestring, codestring_in); }
        bool eventcmp(uint event_in) { return getevent() == event_in; }

        bool filecmp(const char *filename_in) { return filename && !strcmp(filename, filename_in); }

        bool canuninstall() { return !executing; }
        bool& uninstalllater() { return postuninstall; }

        int numcallargs() { return callargidents.length(); }
        bool unneededcallarg(int num) { return !callargidents[num]; }

        const char* getname() { return EVENTNAMES[getevent()]; }
        uint getevent() { return event&0xFFFF; }
        uint geteventcbid() { return (event>>16)&0xFFFF; }

        const char* getfilename() { return filename ? filename : "-"; }

        uint getid() { return id; }
        uint getcalls() { return calls; }
        bool isok() { return ok; }
        void free() { assert(canuninstall()); delete this; }

        bool run(const char *argfmt, va_list args)
        {
            executing = true;
            calls++;
            int retval = ((fcptrint)fcptr)(argfmt, args);
            executing = false;
            return retval != -1;
        }

        bool run(const char *argfmt, va_list args, tagval &result)
        {
            executing = true;
            calls++;
            strtool buf;
            int retval = ((fcptrstr)fcptr)(argfmt, args, buf);
            result.setstr((char*)buf.getbuf());
            buf.disown();
            executing = false;
            return retval != -1;
        }

        void run()
        {
            executing = true;
            calls++;
            ((fcptrvoid)fcptr)();
            executing = false;
        }

        bool run(vector<tagval> &args, tagval &result)
        {
            executing = true;
            calls++;

            identstack callargstack[MAXEVENTARGS];
            int nextcallargstack = 0;
            bool cond = true;

            int numargs = args.length();

            loopi(numargs)
            {
                ident *id = callargidents[i];

                if (id)
                {
                    if (id->flags&IDF_EVENTARG)
                    {
                        id->flags &= ~IDF_UNKNOWN;
                        pusharg(*id, args[i], callargstack[nextcallargstack++]);
                    }
                    else
                    {
                        fatal("event system: file: '%s'  event '%s'  event argument '%s' (argnum: %d)  has no eventarg flag",
                              getfilename(), getname(), id->name, i+1);
                    }
                }
            }

            switch (result.type)
            {
                case VAL_INT:
                {
                    int res = execute(code);
                    cond = (res != -1);

                    if (cond && res != 0)
                        result.setint(res);

                    break;
                }

                case VAL_STR:
                {
                    tagval res;
                    executeret(code, res);

                    if (res.type == VAL_STR && res.s)
                    {
                        if (!*res.s)
                        {
                            delete[] res.s;
                            res.setnull();

                            break;
                        }

                        cond = !(res.s[0] == '-' && res.s[1] == '1');

                        if (result.type == VAL_STR)
                        {
                            if (cond)
                            {
                                delete[] result.s;
                                result.setstr(res.s);
                            }
                            else
                                delete[] res.s;
                        }

                        res.setnull();
                    }
                    break;
                }
            }

            loopi(numargs)
            {
                ident *id = callargidents[i];

                if (id && id->flags&IDF_EVENTARG)
                    poparg(*id);
            }

            executing = false;
            return cond;
        }

        event_t(uint event, const char *argnames, const char *codestring) :
                type(SCRIPT), postuninstall(false), executing(false), filename(getcurrentcsfilename()),
                event(event), codestring(newstring(codestring)), calls(0), fcptr(NULL)
        {
            ok = !argnames || parseargnames(codestring, argnames);

            if (!ok)
            {
                code = NULL;
                return;
            }

            code = compilecode(codestring);
            setid();
        }

        template<class F>
        event_t(uint event, F fcptr) :
                type(evenfcptr<F>::type), ok(true), postuninstall(false), executing(false), filename(NULL),
                event(event), codestring(NULL), code(NULL), calls(0), fcptr((void*)fcptr) { setid(); }

    private:

        void setid()
        {
            static uint nextid = 0;
            id = !availableids.empty() ? availableids.pop() : ++nextid;

            eventcount++;
        }

        bool parseargnames(const char *code, const char *argnames)
        {
            vector<char*> callargs;
            vector<char*> codevars;

            char *buf = newstring(argnames);
            const char *c = strtok(buf, "|, ");

            bool ok = true;

            string error;
            if (!parsevarnames::parsevarnames(codevars, code, error, sizeof(error))) // parse script body for used vars
            {
                erroroutf_r("event %s: failed to parse event args. (%s)", getname(), error);
                ok = false;
                goto end;
            }

            //
            // Now check for vars which we don't need, or we can't use
            // like echo, say, ... etc.
            //

            while (c && callargs.length() < MAXEVENTARGS)
            {
                if (!strncmp(c, "NULL", 4) || !strncmp(c, "null", 4) || !strncmp(c, "ignore", 6))
                {
                    c += (strncmp(c, "null", 4) && strncmp(c, "NULL", 4)) ? 6 : 4;
                    int8_t count = 1;

                    if (isdigit((uchar)*c))
                        count = atoi(c)&0xFF;

                    for(int i = 0; i < count && callargs.length() < MAXEVENTARGS; i++)
                        callargs.add(NULL);
                }
                else
                {
                    ident *id = getident(c);

                    if (id && id->type != ID_ALIAS)
                    {
                        erroroutf_r("event %s: '%s' can't be used as an event argument", getname(), c);

                        ok = false;
                        break;
                    }

                    callargs.add(newstring(c));
                }

                c = strtok(NULL, "|, ");
            }

            if (!ok)
                goto end;

            loopv(callargs)
            {
                if (callargs.length() >= MAXEVENTARGS)
                    break;

                const char *callarg = callargs[i];

                if (callarg)
                {
                    if (!parsevarnames::varexists(codevars, callarg))
                    {
                        if (!eventsystemnounusedcallargwarning)
                        {
                            conoutf("\fs\f3warning:\fr file: %s  event '%s': unneeded variable '%s' detected, "
                                    "use '\fs\f0null\fr' or '\fs\f0ignore\fr' instead if you don't need it",
                                    filename ? filename : "", getname(), callarg);
                        }

                        callargidents.add(NULL);
                        continue;
                    }

                    ident *id = getcsident(callarg);

                    if (!id)
                    {
                        id = addcsident(callarg);
                        id->type = ID_ALIAS;
                    }

                    id->flags |= IDF_EVENTARG;
                    callargidents.add(id);

                    continue;
                }

                callargidents.add(NULL);
            }

            end:;

            delete[] buf;
            callargs.deletearrays();
            codevars.deletearrays();

            return ok;
        }

        ~event_t()
        {
            freecode(code);

            delete[] codestring;

            if (filename)
                delete[] filename;

            if (ok)
                eventcount--;

            availableids.add(id);
        }

        int type;
        bool ok;
        bool postuninstall;
        bool executing;
        char *filename;
        uint event;
        char *codestring;
        uint *code;
        uint id;
        uint calls;
        vector<ident*> callargidents;
        void *fcptr;
    };

    static eventvector_t events[NUMEVENTS];

    static inline eventvector_t& geteventvector(uint event)
    {
        ASSERT(event < NUMEVENTS && "invalid event");
        return events[event];
    }

    UNUSED
    static inline eventvector_t& geteventvector(event_t *event)
    {
        return geteventvector(event->getevent());
    }

    UNUSED
    static inline eventvector_t& geteventvector(const char *event)
    {
        uint eventtype = getevent(event);

#ifdef _DEBUG
        if (eventtype == INVALID) abort();
#endif

        return geteventvector(eventtype);
    }

    static inline eventvector_t* geteventvectorbyid(uint id)
    {
        loopi(NUMEVENTS)
        {
            eventvector_t &v = events[i];

            loopvj(v)
            {
                event_t *event = v[j];

                if (event->cmp(id))
                    return &v;
            }
        }

        return NULL;
    }

    static inline bool isinstalled(uint event, const char *code)
    {
        eventvector_t &v = geteventvector(event);

        loopv(v)
        {
            event_t *cse = v[i];

            if (cse->gettype() == SCRIPT && cse->codecmp(code))
                return true;
        }

        return false;
    }

    static inline int findevents(uint event, eventvector_t &result, uint cbid = 0)
    {
        eventvector_t &v = geteventvector(event);

        if (cbid > 0)
        {
            loopv(v)
            {
                event_t *cse = v[i];

                if (cse->geteventcbid() == cbid)
                {
                    result.add(cse);
                    break;
                }
            }
        }
        else
            result = v;

        return result.length();
    }

    UNUSED
    static inline bool isvalidevent(event_t *event)
    {
        loopj(NUMEVENTS)
        {
            eventvector_t &v = events[j];

            loopv(v)
            {
                if (v[i] == event)
                    return true;
            }
        }

        return false;
    }

    bool isinstalled(uint event)
    {
        eventvector_t &v = geteventvector(event);
        return !v.empty();
    }

    void validate(eventvector_t &events, uint event)
    {
        eventvector_t &v = geteventvector(event);
        events = v;
    }

    static inline void readargs(event_t *cse, const char *args, vector<tagval> &eventargs, va_list vargs)
    {
        eventargs.setsize(0); // clear previous event args

        if (!args)
            return;

        for (const char *a = args; *a; a++)
        {
            if (eventargs.length() >= cse->numcallargs())
                break;

            switch (*a)
            {
                case 'i': case 'd': case 'u':
                {
                    int val = va_arg(vargs, int);
                    eventargs.add().setint(val);
                    break;
                }

                case 'f':
                {
                    double val = va_arg(vargs, double);
                    eventargs.add().setfloat(val);
                    break;
                }

                case 's':
                {
                    const char *val = va_arg(vargs, const char*);

                    if (cse->unneededcallarg(a-args))
                    {
                        eventargs.add().setnull();
                        break;
                    }

                    eventargs.add().setstr(newstring(val));
                    break;
                }

                default:
                    fatal("invalid builtin event argument type. event name: %s", cse->getname());
            }
        }
    }

    static event_t *gcse = NULL;

    int run(tagval &result, uint event, const char *args, va_list vargs)
    {
        static eventvector_t eventscontainer(20);
        eventvector_t *events = &eventscontainer;

        CHECKCALLDEPTH(100, return NO_EVENT_HANDLER);

        uint cbid = (event>>16)&0xFFFF;
        event = event&0xFFFF;

        int eventcount;
        int initeventcount = events->length(); // remember position, need it if it's a nested call

        if (!(eventcount = findevents(event, *events, cbid)))
        {
            //
            // Use static eventscontainer first,
            // even if nested, there may be no events anyway.
            //

            return NO_EVENT_HANDLER;
        }

        if (cd.calldepth > 1)
        {
            //
            // This call is a nested instance of run(),
            // now it would be really dangerous to use the static
            // eventscontainer, because that vector may be changed with
            // every call, so create a new one instead.
            //

            if (initeventcount == eventcount)
                return NO_EVENT_HANDLER;

            events = new eventvector_t(20);

            for(int i = initeventcount; i < eventcount; i++)
                events->add(eventscontainer[i]);

            eventscontainer.setsize(initeventcount);
            eventcount -= initeventcount;

            if (eventscontainer.alen >= 25000)
                eventscontainer.resize();
        }

        static vector<tagval> eventargs(MAXEVENTARGS);

        for(int i = 0; i < eventcount; i++)
        {
            event_t *prevgcse = gcse;
            gcse = (*events)[i]; // global cse, also used in getthiseventid, ...

            if (!gcse) // event was uninstalled, after findevents() was called
            {
                gcse = prevgcse; // restore global event var to previous value
                continue;
            }

            va_list eargs;
            va_copy(eargs, vargs);

            bool res = false;

            switch (gcse->gettype())
            {
                case SCRIPT:
                    readargs(gcse, args, eventargs, eargs);
                    res = gcse->run(eventargs, result);
                    break;

                case FCPTR_INT:
                {
                    va_list v;
                    va_copy(v, eargs);
                    res = gcse->run(args, v);
                    break;
                }

                case FCPTR_STR:
                {
                    va_list v;
                    va_copy(v, eargs);
                    res = gcse->run(args, v, result);
                    break;
                }

                case FCPTR_VOID:
                    gcse->run();
                    res = true;
                    break;
            }

            if (forcecheck)
            {
                //
                // Check will be enforced, in case the user uninstalled events during execution.
                // Every event besides the current executing can be uninstalled, so the last event,
                // will remain valid.
                //

                if (forcecheck == totalmillis)
                {
                    validate(*events, gcse->getevent());

                    if (gcse->uninstalllater()) // user wanted to uninstall this event too, so do so now
                    {
                        //uint id = gcse->getid();
                        gcse->free();
                        eventcount--;

                        //conoutf("\fs\f3event system:\fr event with ID: %u has been post uninstalled", id);
                    }
                }

                forcecheck = (cd.calldepth > 1 ? totalmillis : 0); // nested? so continue checking after returning
            }

            if (!res)
            {
                events->setsize(0);

                if (cd.calldepth > 1) // nested call, so delete dynamic vector
                    delete events;

                gcse = prevgcse; // restore global event var to previous value
                return INTERRUPTED;
            }

            gcse = prevgcse; // restore global event var to previous value
        }

        events->setsize(0);

        if (cd.calldepth > 1) // nested call, so delete dynamic vector
            delete events;

        return 0;
    }

    int run(uint event, const char *args, ...)
    {
        tagval result;
        result.type = VAL_INT;
        result.i = 0;

        va_list vargs;
        va_start(vargs, args);

        int res;
        if ((res = run(result, event, args, vargs)))
            return res;

        return result.i;
    }

    int run(char *buf, size_t size, uint event, const char *args, ...)
    {
        tagval result;

        result.type = VAL_STR;
        result.s = NULL;

        va_list vargs;
        va_start(vargs, args);

        int res;
        if ((res = run(result, event, args, vargs)))
        {
            if (result.s)
            {
                delete[] result.s;
                result.setnull();
            }

            return res;
        }

        if (!result.s)
            return NO_RESULT;

        copystring(buf, result.s, size);
        delete[] result.s;
        result.setnull();

        return RESULT;
    }

    static void showwarningmsg(strtool *msg, uint)
    {
        conoutf(CON_WARN, "%s", msg->str());
        delete msg;
    }

    uint install(const char *name, const char *argnames, const char *code)
    {
        CHECKCALLDEPTH(100, return 0);

        if (!*code)
        {
            code = argnames; // argnames can be skipped, when not needed
            argnames = NULL;
        }

        if (!*name)
        {
            mod::erroroutf_r("%s(): no event name given!", __FUNCTION__);
            return 0;
        }

        if (!*code)
        {
            mod::erroroutf_r("%s(): no codeblock given!", __FUNCTION__);
            return 0;
        }

        if (eventcount >= MAXEVENTS)
        {
            conoutf("\fs\f3event system:\fr reached maximum events limit (%u)", MAXEVENTS);
            return 0;
        }

        uint eventtype = getevent(name);

        if (eventtype == INVALID)
        {
            erroroutf_r("no such event '%s'", name);
            return 0;
        }

        if (!eventsystemnoduplicatewarning && isinstalled(eventtype, code))
            conoutf("\fs\f3warning:\fr event '%s': is a duplicate of an already installed event (same type and code)", name);

        eventvector_t &v = geteventvector(eventtype);

        event_t* event = v.add(new event_t(eventtype, argnames, code));
        forcecheck = totalmillis;

        if (!event->isok())
        {
            v.pop();
            event->free();

            intret(0);
            return 0;
        }

        for (const warnmsg *w = &warn[0]; w->event != INVALID; w++)
        {
            if (w->event == eventtype)
            {
                const char *filename = getcurrentcsfilenamep();

                if (!filename)
                    filename = "unknown";

                strtool *msg = new strtool(sizeof(string)*2);
                msg->fmt("\fs\f3warning:\fr %s event installed from file %s (\fs\f3%s\fr)!", EVENTNAMES[w->event], filename, w->msg);

                seedMT(getnanoseconds()&0xFFFFFFFF);
                addtimerfunction(500+rnd(rnd(3)>1?1000:3000), true, (void (*)(void*,uint))&showwarningmsg, msg);

                break;
            }
        }

        forcecheck = totalmillis;

        intret((int)event->getid());
        return event->getid();
    }

    static inline bool havecallbackid(uint id)
    {
        eventvector_t &v = geteventvector(INTERNAL_CALLBACK);

        loopv(v)
        {
            event_t *cse = v[i];

            if (cse->geteventcbid() == id)
                return true;
        }

        return false;
    }

    uint install(uint eventtype, const char *argnames, const char *code, uint *callbackid)
    {
        eventvector_t &v = geteventvector(eventtype);
        uint cbid = 0;

        if (eventtype == INTERNAL_CALLBACK && callbackid)
        {
            static const uint IDMIN = 1;
            static const uint IDMAX = 0xFFFD;

            cbid = IDMIN;

            while (cbid < IDMAX)
            {
                if (!havecallbackid(cbid))
                    break;

                cbid++;
            }

            if (cbid == IDMAX)
                return 0;

            *callbackid = cbid;
        }

        event_t *event = v.add(new event_t(cbid<<16|eventtype, argnames, code));
        forcecheck = totalmillis;

        if (!event->isok())
        {
            v.pop();
            event->free();

            return 0;
        }

        return event->getid();
    }

    template<class T>
    uint install(uint eventtype, T fcptr)
    {
        if (eventcount >= MAXEVENTS)
            return 0;

        eventvector_t &v = geteventvector(eventtype);
        event_t *event = v.add(new event_t(eventtype, fcptr));

        if (!event->isok())
        {
            v.pop();
            event->free();

            return 0;
        }

        return event->getid();
    }

    uint install(uint eventtype, fcptrint fcptr)
    {
        return install<>(eventtype, fcptr);
    }

    uint install(uint eventtype, fcptrstr fcptr)
    {
        return install<>(eventtype, fcptr);
    }

    uint install(uint eventtype, fcptrvoid fcptr)
    {
        return install<>(eventtype, fcptr);
    }

    void uninstall(uint id, bool msg, bool calledfromscript)
    {
        CHECKCALLDEPTH(100, return);

        eventvector_t *v = geteventvectorbyid(id);

        if (!v)
            return;

        loopvrev(*v)
        {
            event_t *cse = (*v)[i];

            if (cse->cmp(id))
            {
                if (calledfromscript && cse->gettype() != SCRIPT)
                {
                    if (msg)
                        erroroutf_r("event system: this event is used internally and can't be uninstalled");

                    continue;
                }

                if (!cse->canuninstall())
                {
                    v->remove(i);

                    cse->uninstalllater() = true;

                    if (msg)
                        erroroutf_r("event system: this event can't be uninstalled right now");

                    forcecheck = totalmillis;

                    intret(0);
                    return;
                }

                v->remove(i);
                cse->free();

                forcecheck = totalmillis;

                intret(1);
                return;
            }
        }

        intret(0);
    }

    static void uninstallall()
    {
        loopj(NUMEVENTS)
        {
            eventvector_t &v = events[j];
            bool internalreferences = false;

            loopvrev(v)
            {
                event_t *cse = v[i];

                if (cse->gettype() != SCRIPT)
                {
                    internalreferences = true;
                    continue;
                }

                if (!cse->canuninstall())
                {
                    cse->uninstalllater() = true;
                    erroroutf_r("event system: event with id: %u can't be uninstalled right now", cse->getid());
                    continue;
                }

                cse->free();
                v.remove(i);
            }

            if (!internalreferences)
                v.setsize(0);
        }

        forcecheck = totalmillis;
    }

    void startup()
    {
    }

    void shutdown()
    {
        uninstallall();
    }

    void scriptfileloading(const char *filename)
    {
        //
        // Automatically unload events when a file gets loaded again,
        // to prevent having events installed twice.
        //

        CHECKCALLDEPTH(100, return);

        loopj(NUMEVENTS)
        {
            eventvector_t &v = events[j];

            loopvrev(v)
            {
                event_t *cse = v[i];

                if (cse->filecmp(filename))
                {
                    conoutf("\fs\f3event system:\fr file reload (\fs\f0%s\fr): automatically uninstalled event "
                            "'\fs\f0%s\fr' - id: \fs\f0%u\fr", filename, cse->getname(), cse->getid());

                    v.remove(i);
                    cse->free();

                    forcecheck = totalmillis;
                }
            }
        }
    }

    void scriptfileloaded(const char *filename)
    {
        //
        // Call corresponding "scriptinit"'s whenever a
        // script file was fully processed.
        //

        eventvector_t events;

        int count;
        if (!(count = findevents(SCRIPT_INIT, events)))
            return;

        vector<tagval> args;

        tagval result;
        result.type = VAL_INT;

        for(int i = 0; i < count; i++)
        {
            event_t *cse = events[i];

            if (cse->filecmp(filename))
            {
                cse->run(args, result);

                if (cse->uninstalllater())
                    cse->free();
                else
                    uninstall(cse->getid());
            }

            validate(events, SCRIPT_INIT);
        }
    }

    static void listevents()
    {
        if (!eventcount)
        {
            conoutf("no events installed");
            return;
        }

        conoutf("listing %u installed event%s ...", eventcount, plural(eventcount));

        loopj(NUMEVENTS)
        {
            eventvector_t &v = events[j];

            if (v.empty())
                continue;

            conoutf("\f4chain: %s (%d event%s):", EVENTNAMES[j], v.length(), plural(v.length()));

            loopv(v)
            {
                event_t *cse = v[i];
                const char *filename;

                if (cse->gettype() == SCRIPT)
                    filename = cse->getfilename();
                else
                    filename = "internal/c++";

                conoutf("\fs\f0event name\fr: %s (%s)  \fs\f0calls:\fr %u  (id: %u)",
                        cse->getname(), filename, cse->getcalls(), cse->getid());
            }
        }
    }

    ICOMMAND(scriptevent, "sss", (const char *name, const char *argnames, const char *code), install(name, argnames, code));
    ICOMMAND(delscriptevent, "i", (int *id), uninstall(*id, true, true));
    COMMANDN(delallscriptevents, uninstallall, "");
    ICOMMAND(getscripteventcount, "", (), intret((int)eventcount));
    ICOMMAND(getthiseventid, "", (), intret(int(gcse ? gcse->getid() : 0)));
    COMMANDN(listscriptevents, listevents, "");
} // namespace event
} // namespace mod
