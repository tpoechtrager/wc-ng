/***********************************************************************
 *  WC-NG G15 LCD Plugin                                               *
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

#include "cube.h"
#include "g15sdkwrapper.h"

#ifndef _WIN32
typedef void* HANDLE;
#endif //!_WIN32

using namespace mod::plugin;

namespace g15lcd
{
    static const hostexports_t *host;

    static void *lcd = NULL;

    static const size_t LINES = 3;
    static HANDLE texthandles[LINES];        // text handle for each line

    static SDL_cond *updatecond = NULL;      // condition for flushing text to keyboard
    static SDL_mutex *updatelock = NULL;     // lock for updatecond, not used for anything else
    static SDL_Thread *updatethread = NULL;
    static SDL_mutex *textlock = NULL;       // lock for the text buffer

    static const size_t MAXLCDTEXTLEN = 120; // max line length */
    static char lcdtext[MAXLCDTEXTLEN*LINES+LINES*sizeof(size_t)] = {0}; // aligned buffer for all lines + \0

    static bool exitthread = false;

    static inline void inittexthandles()
    {
        loopi(LINES)
        {
            HANDLE texthandle = createtexthandle(lcd);
            if (!texthandle) abort();
            texthandles[i] = texthandle;
        }
    }

    static void drawtext(int line, const char *text)
    {
        if (!line || sizeofarray(texthandles) < (uint)line)
            return;

        size_t len = strlen(text)+1;
        char *p = &lcdtext[--line*sizeof(lcdtext)/LINES]; /* get a pointer to the wanted line buffer */

        {
            /*
             * lock and copy text to line buffer
             */
            SDL_Mutex_Locker m(textlock);
            memcpy(p, text, min(MAXLCDTEXTLEN, len));
            p[MAXLCDTEXTLEN] = '\0';
        }
    }

    static int update(void *)
    {
        char textbuf[sizeof(lcdtext)];
        SDL_mutexP(updatelock);

        while (true)
        {
            SDL_CondWait(updatecond, updatelock);

            if (exitthread)
            {
                SDL_mutexV(updatelock);
                break;
            }

            {
                /*
                 * avoid long locking by copying the global
                 * buffer into a local one
                 */
                SDL_Mutex_Locker m(textlock);
                memcpy(textbuf, lcdtext, sizeof(lcdtext));
            }

            /*
             * prepare text for each line
             */
            loopi(LINES)
            {
                const char *text = &textbuf[i*sizeof(lcdtext)/LINES];
                HANDLE texthandle = texthandles[i];

                settext(lcd, texthandle, text);
                setorigin(lcd, texthandle, 0, i*15);
            }

            /*
             * flush text to keyboard
             */
            updatedisplay(lcd);
        }

        return 0;
    }

    static inline void updatelcd()
    {
        SDL_CondSignal(updatecond);
    }

    static bool command(const char *cmd, const char *val)
    {
        #define cmdcmp(p, cmd) !strcmp(p, &cmd[1])

        switch (*cmd++)
        {
            case 'd':
            {
                if (!val[0]) return false;
                if (cmdcmp(cmd, "drawtext"))
                {
                    int line = atoi(val);
                    const char *p = val;
                    while (*p && *p != ' ') p++;
                    drawtext(line, p);
                    return true;
                }

                break;
            }
            case 'u':
                if (cmdcmp(cmd, "updatelcd")) { updatelcd(); return true; }
                break;
        }

        #undef cmdcmp

        return false;
    }

    const char *const COMMANDNAMES[] =
    {
        "drawtext", "updatelcd", NULL
    };


    static void cleanup()
    {
        if (lcd) freelcdhandle(&lcd);

        Destroy_SDL_Mutex(updatelock);
        Destroy_SDL_Cond(updatecond);
        Destroy_SDL_Mutex(textlock);

        updatethread = NULL;
    }

    static unload_code_t unload()
    {
        SDL_mutexP(updatelock);
        exitthread = true;
        SDL_mutexV(updatelock);

        SDL_CondSignal(updatecond);
        SDL_WaitThread(updatethread, NULL);

        cleanup();
        return PLUGIN_UNLOAD_OK;
    }

    EXPORT init_code_t init(const hostexports_t *hostexports, plugin_t *plugin)
    {
        host = hostexports;

        if (!checkapiversion(host))
            return PLUGIN_INIT_ERROR_PLUGIN_API;

        if (!(updatecond = SDL_CreateCond()) ||
            !(updatelock =  SDL_CreateMutex()) ||
            !(updatethread = SDL_CreateThread(update, "", NULL)) ||
            !(textlock = SDL_CreateMutex()))
        {
            host->client.erroroutf_r("g15lcd: failed to create required ressources");
            cleanup();
            return PLUGIN_INIT_ERROR;
        }

        if (!(lcd = createnewlcdhandle()) || !initlcdhandle(lcd, "Sauerbraten") || !isdevicepresent(lcd))
        {
#ifdef _WIN32
            static const char *const text = "g15lcd: no G15 Keyboard present, or driver not installed";
#else
            static const char *const text = "g15lcd: no G15 Keyboard present, or g15 daemon isn't running";
#endif //_WIN32

            host->client.conoutf(CON_ERROR, text);
            unload();
            return PLUGIN_INIT_ERROR;
        }

        inittexthandles();

        drawtext(2, "Sauerbraten G15 Plugin");
        updatelcd();

        plugin->version = plugin_version_t(0, 4, 0);
        plugin->clientver = CLIENTVERSION;
        plugin->clientrev = WCREVISION;
        plugin->unload = unload;
        plugin->command = command;
        plugin->commandnames = COMMANDNAMES;

        return PLUGIN_INIT_OK;
    }
}
