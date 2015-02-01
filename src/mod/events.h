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

namespace mod {
namespace event
{
    enum
    {
        // PLAYER
        PLAYER_CONNECT, PLAYER_DISCONNECT, PLAYER_RENAME, PLAYER_JOIN_SPEC, PLAYER_LEAVE_SPEC,
        PLAYER_SWITCH_TEAM, PLAYER_TEXT, PLAYER_TEAM_TEXT, PLAYER_FRAG, PLAYER_TEAM_KILL,
        PLAYER_PING_UPDATE,

        // CTF
        FLAG_SCORE, FLAG_DROP, FLAG_TAKE, FLAG_RETURN, FLAG_RESET, COAF,

        // COLLECT
        SKULL_SCORE, SKULL_TAKE, COAS,

        // CAPTURE
        BASE_CAPTURED, BASE_LOST, COAB,

        // MISC OTHERS
        STARTUP, SHUTDOWN, FRAME, MAPSTART, INTERMISSION, SERVER_MSG, MASTER_UPDATE,
        MASTERMODE_UPDATE, CONSOLE_INPUT, IPIGNORELIST, SERVCMD,

        // PLUGIN
        PLUGIN_LOAD, PLUGIN_UNLOAD,

        // HWTEMP PLUGIN

        HWTEMP_INTERFACE, HWTEMP,

        // DEMOPLAYBACK
        DEMO_START, DEMO_END, DEMOMILLISJUMP_START, DEMOMILLISJUMP_END,

        // EXTINFO
        SERVER_MOD, EXTINFO_UPDATE, EXTINFO_COUNTRY_UPDATE,

        // NETWORK
        CONNECT, DISCONNECT, BANDWIDTH_UPDATE,

        // EVENT SYSTEM
        SCRIPT_INIT, GENERIC, INTERNAL_CALLBACK,

        // DEMO
        CLIENT_DEMO_START, CLIENT_DEMO_END,

        // GUI
        SHOW_GUI, CLOSE_GUI,

        NUMEVENTS
    };

    const char *const EVENTNAMES[] =
    {
        // PLAYER
        "playerconnect", "playerdisconnect", "playerrename", "playerjoinspec", "playerleavespec",
        "playerswitchteam", "playertext", "playerteamtext", "playerfrag", "playerteamkill",
        "playerpingupdate",

        // CTF
        "flagscore", "flagdrop", "flagtake", "flagreturn", "flagreset", "coaf",

        // COLLECT
        "skullscore", "skulltake", "cpluginloadoas",

        // CAPTURE
        "basecaptured", "baselost", "coab",

        // MISC OTHER
        "startup", "shutdown", "frame", "mapstart", "intermission", "servmsg", "masterupdate",
        "mastermodeupdate", "consoleinput", "ipignorelist", "servcmd",

        // PLUGIN
        "pluginload", "pluginunload",

        // HWTEMP PLUGIN

        "hwtemp_interface", "hwtemp",

        // DEMOPLAYBACK
        "demostart", "demoend", "demomillisjump_start", "demomillisjump_end",

        // EXTINFO
        "servermod", "extinfoupdate", "countryupdate",

        // NETWORK
        "connect", "disconnect", "bandwidthupdate",

        // EVENT SYSTEM
        "scriptinit", "generic", "",

        // DEMO
        "clientdemostart", "clientdemoend",

        // GUI
        "showgui", "closegui"
    };

    static_assert(sizeofarray(EVENTNAMES) == NUMEVENTS, "event tables do not match");

    static uint INVALID = (uint)-1;

    static inline uint getevent(const char *const event)
    {
        if (*event)
        {
            loopi(NUMEVENTS)
            {
                if (!strcmp(EVENTNAMES[i], event))
                    return i;
            }
        }

        return INVALID;
    }

    static inline uint getevent(uint event)
    {
        if (event >= NUMEVENTS) return INVALID;
        return event;
    }

    static struct eventsystem_t
    {
        size_t size;
        int NUMEVENTS;
        eventsystem_t() : size(sizeof(*this)), NUMEVENTS(::mod::event::NUMEVENTS) {}
        bool operator==(const eventsystem_t &in) { return (size == in.size && in.NUMEVENTS == NUMEVENTS); }
    } eventsystem;

    static inline bool validateeventsystem(const eventsystem_t &evs)
    {
        return eventsystem == evs;
    }

    enum
    {
        SCRIPT,
        FCPTR_INT,
        FCPTR_STR,
        FCPTR_VOID
    };

    //
    // FCPTR types
    // Return -1 if no further events should be called
    // otherwise return 0
    //

    typedef int (*fcptrint)(const char *argfmt, va_list args);
    typedef int (*fcptrstr)(const char *argfmt, va_list args, strtool &result);
    typedef void (*fcptrvoid)();

    template<class T> struct evenfcptr;
    template<> struct evenfcptr<fcptrint> { static const int type = FCPTR_INT; };
    template<> struct evenfcptr<fcptrstr> { static const int type = FCPTR_STR; };
    template<> struct evenfcptr<fcptrvoid> { static const int type = FCPTR_VOID; };

    static inline fcptrint l2f(fcptrint f){ return f; }
    static inline fcptrstr l2f(fcptrstr f){ return f; }
    static inline fcptrvoid l2f(fcptrvoid f){ return f; }
    
    enum
    {
        NO_RESULT         = -0x7FFFFFFA, // string only
        RESULT            = -0x7FFFFFFB, // string only
        NO_EVENT_HANDLER  = -0x7FFFFFFC,
        INTERRUPTED       = -0x7FFFFFFD
    };

    void startup();
    void shutdown();
    void scriptfileloading(const char *filename);
    void scriptfileloaded(const char *filename);
    bool isinstalled(const char *event);

#ifdef __clang__
    #pragma clang diagnostic ignored "-Wunnamed-type-template-args"
#endif //__clang__

    uint install(const char *name, const char *argnames, const char *code);
    uint install(uint event, const char *argnames, const char *code, uint *callbackid);

    uint install(uint eventtype, fcptrint fcptr);
    uint install(uint eventtype, fcptrstr fcptr);
    uint install(uint eventtype, fcptrvoid fcptr);

    template<class T, class F>
    uint install(T event, F fcptr)
    {
        CHECKCALLDEPTH(2, fatal("fcptr must match to the fcptr types"));
        uint eventtype = getevent(event);
        if (eventtype == INVALID) fatal("invalid event");
        return install(eventtype, fcptr);
    }

    template<class T, class F>
    uint xinstall(T event, F fcptr)
    {
        uint id = install<T, F>(event, fcptr);
        if (!id) fatal("internal event installation failed");
        return id;
    }

    void uninstall(uint id, bool msg = true, bool calledfromscript = false);
    int run(uint event, const char *args = NULL, ...);
    int run(char *buf, size_t size, uint event, const char *args, ...); // string result - check against RESULT (enum)
} // namespace event
} // namespace mod
