/***********************************************************************
 *  WC-NG HWTEMP Plugin                                                *
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

#include "hwtemp.h"

namespace hwtemp
{
    const hostexports_t *host;

    static int gettemps(void *);                      // fwd decl

    static sdlcondition gettempcond;
    static sdlmutex gettemplock;                      // lock for gettemp cond, not used for anything else
    static sdlthread gettempthread(gettemps);         // thread for getting hardware temps
    static sdlmutex templock;                         // lock for protecting variables

    static const int MAXQUEUE = 32;
    static queue<temps, MAXQUEUE> tempqueue;          // synchronisation queue
    static mod::strtool interface("generic");

    static atomic<bool> exitthread(false);

    static inline int gettemp(temps &t)
    {
        loopi(NINTERFACES)
        {
            const struct interface &iface = INTERFACES[i];

            if (iface == t.interface)
            {
                if (iface(t))
                    return GETTEMP_OK;

                return GETTEMP_ERROR;
            }
        }

        return GETTEMP_INVALID_INTERFACE;
    }

    static int gettemps(void *)
    {
        temps ltemp;
        gettemplock.lock();

        while (true)
        {
            gettempcond.wait(gettemplock);

            if (exitthread)
            {
                gettemplock.unlock();
                break;
            }

            {
                SDL_Mutex_Locker m(templock);
                copystrtool(ltemp.interface, interface);
            }

            ltemp.reset();
            switch (gettemp(ltemp))
            {
                case GETTEMP_OK:
                {
                    SDL_Mutex_Locker m(templock);
                    if (tempqueue.full())
                    {
                        host->client.conoutf_r(CON_WARN, "hwtemp plugin: warning: temperature queue is full!");
                        break;
                    }
                    tempqueue.add() = ltemp;
                    break;
                }
                case GETTEMP_INVALID_INTERFACE:
                {
                    SDL_Mutex_Locker m(templock);
                    host->client.erroroutf_r("hwtemp plugin: invalid interface: %s", ltemp.interface.str());
                    break;
                }
                case GETTEMP_ERROR:
                {
                    SDL_Mutex_Locker m(templock);
                    host->client.erroroutf_r("hwtemp plugin: unable to read temperatures from (%s): "
                                             "client (CoreTemp, SpeedFAN, ...) not running or device not present?", ltemp.interface.str());
                    break;
                }
            }

            if (exitthread)
            {
                gettemplock.unlock();
                break;
            }
        }

        return 0;
    }

    static inline void gettemps()
    {
        gettempcond.signalone();
    }

    static bool command(const char *cmd, const char *val)
    {
        if (!strcmp(cmd, "interface"))
        {
            SDL_Mutex_Locker m(templock);
            interface = val;
            return true;
        }
        else if (!strncmp(cmd, "gettemp", STRLEN("gettemp")))
        {
            gettemps();
            return true;
        }

        return false;
    }

    const char *const COMMANDNAMES[] =
    {
        "interface", "gettemp", NULL
    };

    void slice()
    {
        SDL_Mutex_Locker m(templock);

        while (!tempqueue.empty())
        {
            temps &ts = tempqueue.remove();

            host->event.run(mod::event::HWTEMP_INTERFACE, "sd", ts.interface.str(), ts.ntemperatures);

            loopi(ts.ntemperatures)
            {
                temp &t = ts.temperatures[i];
                host->event.run(mod::event::HWTEMP, "sffd", t.desc, t.temperature, t.load, i+1);
            }
        }
    }

    static unload_code_t unload()
    {
        exitthread = true;
        gettempcond.signalone();
        gettempthread.join();
        return PLUGIN_UNLOAD_OK;
    }

    EXPORT init_code_t init(const hostexports_t *hostexports, plugin_t *plugin)
    {
        host = hostexports;

        if (!checkapiversion(host))
            return PLUGIN_INIT_ERROR_PLUGIN_API;

        if (!host->event.validateeventsystem(mod::event::eventsystem))
        {
            host->client.erroroutf_r("plugin hwtemp is oudated; please get a newer version");
            return PLUGIN_INIT_ERROR;
        }

        plugin->version = plugin_version_t(0, 4, 9);
        plugin->clientver = CLIENTVERSION;
        plugin->clientrev = WCREVISION;
        plugin->unload = unload;
        plugin->slice = slice;
        plugin->command = command;
        plugin->commandnames = COMMANDNAMES;

        return PLUGIN_INIT_OK;
    }
}
