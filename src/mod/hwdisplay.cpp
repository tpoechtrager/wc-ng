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

using namespace game;
using namespace mod;

extern int fullconsole;
extern strtool getgfxvendor();
bool disablehwdisplay = false;

namespace gamemod
{
    static void showdisplay();
    static int hwtemphook(const char *argfmt, va_list args);
    static int hwtempinterfacehook(const char *argfmt, va_list args);
    static void framehook();
    static int pluginunloadhook(const char *argfmt, va_list args);
    static void shutdownhook();

    static const char *const HWTEMPPLUGINNAME = "hwtemp";

    enum
    {
        TEMP_NVIDIA_GPU,
        TEMP_ATI_GPU,
        TEMP_GENERIC
    };

    static const char *const INTERFACES[] =
    {
        "nvgpu",
        "atigpu",
        "generic"
    };

    static inline const char *getinterfacename(int interface)
    {
        return INTERFACES[interface];
    }

    static const int stdfontscale = 25;
    static const int stdrightoffset = 0;
    static const int stdheightoffset = 250;
    static const int stdwidthoffset = 0;
    static const int stdlineoffset = 0;
    static const int stdalpha = 150;

    static float fontscale = stdfontscale/100.0f;

    MODVARFP(showhwdisplay, 0, 0, 1, showdisplay());
    MODVARFP(hwdisplayfontscale, 25, stdfontscale, 27, fontscale = hwdisplayfontscale/100.0f);
    MODVARP(hwdisplayrightoffset, -200, stdrightoffset, 200);
    MODVARP(hwdisplayheightoffset, 0, stdheightoffset, 600);
    MODVARP(hwdisplaywidthoffset, -300, stdwidthoffset, 300);
    MODVARP(hwdisplaylineoffset, -30, stdlineoffset, 50);
    MODVARP(hwdisplayalpha, 20, stdalpha, 0xFF);
    MODVARP(hwdisplayfahrenheit, 0, 0, 1);
    MODVARP(hwdisplayalarmthreshold, -1, -1, 500);
    MODVARP(hwdisplayupdateinterval, 250, 500, 5000);
    MODSVARP(hwdisplayblacklist, "");

    static inline bool isblacklisted(const char *device)
    {
        if (!*hwdisplayblacklist)
            return false;

        static const char *d = ";:,";
        char *buf = newstring(hwdisplayblacklist);
        const char *p = strtok(buf, d);

        while (p)
        {
            if (!strcmp(p, device))
            {
                delete[] buf;
                return true;
            }

            p = strtok(NULL, d);
        }

        delete[] buf;
        return false;
    }

    static void resettempnames();

    static void hwdisplayreset()
    {
        hwdisplayfontscale = stdfontscale;
        hwdisplayrightoffset = stdrightoffset;
        hwdisplayheightoffset = stdheightoffset;
        hwdisplaywidthoffset = stdwidthoffset;
        hwdisplaylineoffset = stdlineoffset;
        hwdisplayalpha = stdalpha;
        resettempnames();
    }
    COMMAND(hwdisplayreset, "");

    static const char *interfacename = NULL;
    static int tempsfollowing = 0;
    static int requested = 0;

    struct temp
    {
        string name;
        float temperature;
    };

    struct tempname
    {
        strtool oldname;
        strtool newname;

        tempname &operator=(tempname &in)
        {
            oldname.swap(in.oldname);
            newname.swap(in.newname);
            return *this;
        }

        tempname(const char *oldname, const char *newname) :
            oldname(oldname), newname(newname) {}
    };

    static vector<temp> temps;
    static vector<temp> ctemps;
    static vector<tempname> tempnames;
    static vector<uint> hookids;
    static vector<const char*> interfaces;
    static int curinterface = 0;

    static inline void resettempnames()
    {
        tempnames.shrink(0);
    }

    static inline const char *lookuptempname(const char *in)
    {
        if (const char *p = strchr(in, '@'))
        {
            if ((p = strchr(p, '-')))
            {
                static strtool buf;
                buf.copy(in, p-in);
                in = buf.str();
            }
        }

        if (isblacklisted(in))
            return NULL;

        loopv(tempnames)
        {
            tempname &tn = tempnames[i];

            if (tn.oldname == in)
                return tn.newname.str();
        }

        return in;
    }

    static void addhwdisplaydevicealias(const char *oldname, const char *newname)
    {
        if (!*oldname || !*newname)
        {
            erroroutf_r("old and new name must be given!");
            intret(0);
        }

        loopv(tempnames) if (tempnames[i].oldname == oldname)
        {
            tempnames[i].newname = newname;
            intret(1);
            return;
        }

        tempnames.add(tempname(oldname, newname));
        intret(1);
    }
    COMMAND(addhwdisplaydevicealias, "ss");

    static void delhwdisplaydevicealias(const char *name)
    {
        loopv(tempnames)
        {
            if (tempnames[i].oldname == name || tempnames[i].newname == name)
            {
                tempnames.remove(i);
                intret(1);
                return;
            }
        }

        intret(0);
    }
    COMMAND(delhwdisplaydevicealias, "s");

    void writetempnames(stream *f)
    {
        if (tempnames.empty())
            return;

        f->printf("\n");

        loopv(tempnames)
        {
            tempname &tn = tempnames[i];

            f->printf("addhwdisplaydevicealias %s %s\n",
                      escapestring(tn.oldname.str()),
                      escapestring(tn.newname.str()));
        }
    }

    static inline bool installhooks()
    {
        hookids.add() = event::install<int>(event::HWTEMP_INTERFACE, hwtempinterfacehook);
        hookids.add() = event::install<int>(event::HWTEMP, hwtemphook);
        hookids.add() = event::install<int>(event::FRAME, framehook);
        hookids.add() = event::install<int>(event::PLUGIN_UNLOAD, pluginunloadhook);
        hookids.add() = event::install<int>(event::SHUTDOWN, shutdownhook);

        loopv(hookids) if (!hookids[i]) return false;
        return true;
    }

    static inline void uninstallhooks()
    {
        loopv(hookids)
        {
            uint hid = hookids[i];
            if (hid) event::uninstall(hid, false);
        }
        hookids.setsize(0);
    }

    static inline void findinterfaces()
    {
        strtool gfxvendor = getgfxvendor();

        if (gfxvendor.find("NVIDIA"))
            interfaces.add(getinterfacename(TEMP_NVIDIA_GPU));
        else if (gfxvendor.find("ATI"))
            interfaces.add(getinterfacename(TEMP_ATI_GPU));


        interfaces.add(getinterfacename(TEMP_GENERIC));
    }

    static inline void resetvariables()
    {
        hookids.setsize(0);
        temps.setsize(0);
        ctemps.setsize(0);
        resettempnames();
        interfacename = NULL;
        tempsfollowing = 0;
        requested = 0;
        interfaces.setsize(0);
        curinterface = 0;
    }

    static void showdisplay()
    {
        bool isinitialized = hookids.length() != 0;

        if (showhwdisplay)
        {
            if (isinitialized)
                return;

            if (!plugin::require(HWTEMPPLUGINNAME))
                return;

            resetvariables();

            if (!installhooks())
            {
                conoutf("couldn't initialize hwtemp display");
                uninstallhooks();
                return;
            }

            findinterfaces();

            return;
        }
        else if (isinitialized)
        {
            uninstallhooks();
        }
    }

    static int hwtempinterfacehook(const char *argfmt, va_list args)
    {
        assert(!strcmp(argfmt, "sd") && "incompatible plugin");

        requested--;
        interfacename = va_arg(args, const char*);
        tempsfollowing = va_arg(args, int);

        if (temps.empty())
            temps.reserve(tempsfollowing);

        return 0;
    }

    static int hwtemphook(const char *argfmt, va_list args)
    {
        assert(!strcmp(argfmt, "sffd") && "incompatible plugin");

        if (tempsfollowing <= 0)
            return 0;

        const char *name = lookuptempname(va_arg(args, const char*));

        if (name)
        {
            temp &t = temps.add();
            copystring(t.name, name);
            t.temperature = va_arg(args, double);
        }

        if (!--tempsfollowing && !strcmp(interfacename, interfaces.last()))
        {
            swap(temps, ctemps);
            temps.setsize(0);
        }

        return 0;
    }

    static void framehook()
    {
        static int last = 0;

        if (interfaces.empty())
            return;

        if (!requested && (!last || totalmillis-last >= hwdisplayupdateinterval/interfaces.length()))
        {
            assert(!tempsfollowing);

            const char *iname = interfaces[curinterface++];
            plugin::command(HWTEMPPLUGINNAME, "interface", iname);
            plugin::command(HWTEMPPLUGINNAME, "gettemp", NULL);

            if (curinterface >= interfaces.length())
                curinterface = 0;

            requested++;
            last = totalmillis;
        }
    }

    static void pluginunload()
    {
        uninstallhooks();
        resetvariables();
        showdisplay();
    }

    static int pluginunloadhook(const char *argfmt, va_list args)
    {
        const char *plugin = va_arg(args, const char*);
        int isshutdown = va_arg(args, int);

        if (!isshutdown && !strcmp(plugin, HWTEMPPLUGINNAME))
        {
            conoutf("%s plugin has been unloaded unexpectedly, will load it again ...", HWTEMPPLUGINNAME);

            uninstallhooks();
            resetvariables();

            addtimerfunction(0, true, &pluginunload);
        }

        return 0;
    }

    static void shutdownhook()
    {
        uninstallhooks();
    }

    static inline double celsius2fahrenheit(double celsius)
    {
        return celsius*9.0/5.0+32.0;
    };

    void renderhwdisplay(int conw, int conh, int FONTH, int w, int h)
    {
        if ((disablehwdisplay || !showhwdisplay) ||
            (isscoreboardshown() || !isconnected(false, true)))
             return;

        if (fullconsole || w < 1024 || h < 640)
            return;

        int lw[] = { 2800, 2500 };

        if (!m_ctf && !m_capture)
        {
            for (int i=0; i<(int)sizeofarray(lw); i++)
            {
                int &w = lw[i];
                w += 140;
            }
        }

        glPushMatrix();
        glScalef(fontscale, fontscale, 1);

        loopv(ctemps)
        {
            temp &t = ctemps[i];

            int temp = t.temperature;
            char symbol = 'C';

            if (hwdisplayfahrenheit)
            {
                temp = (int)celsius2fahrenheit((double)temp);
                symbol = 'F';
            }

            int left;
            int top = conh;

            top += rescaleheight(-100+LINEOFFSET(i)+hwdisplaylineoffset*i+hwdisplayheightoffset, h);

            static draw dt;

            dt.clear();
            dt.setalpha(hwdisplayalpha);

            if (hwdisplayalarmthreshold != -1 && temp >= hwdisplayalarmthreshold)
            {
                dt.setcolour(0xFF, 0x00, 0x00);
                dt.setalpha(0xFF);
            }

            left = conw+rescalewidth(lw[1]+hwdisplaywidthoffset, w);
            dt << t.name;
            dt.drawtext(left , top);

            dt.cleartext();

            left = conw+rescalewidth(lw[0]+hwdisplaywidthoffset+hwdisplayrightoffset, w);
            dt << temp << ' ' << symbol;
            dt.drawtext(left, top);
        }

        glPopMatrix();
    }
}
