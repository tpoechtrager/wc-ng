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

#include "cube.h"

using namespace mod::plugin;

namespace hwtemp
{
    extern const hostexports_t *host;

    struct temp
    {
        string desc;          // description of device
        float temperature;    // temperature of device in °c
        float load;           // load of device in %
    };

    struct temps
    {
        static const int MAXTEMPS = 32;
        mod::strtool interface;          // interface to use
        temp temperatures[MAXTEMPS];     // temperature queue
        int ntemperatures;               // count of temperatures

        bool exists(const char *devicename)
        {
            size_t len = strlen(devicename);

            loopi(ntemperatures)
            {
                temp &t = temperatures[i];

                if (!strncmp(t.desc, devicename, len))
                    return true;
            }

            return false;
        }

        void reset()
        {
            ntemperatures = 0;
            memset(&temperatures, 0, sizeof(temperatures));
        }
        temps() { reset(); }
    };

    enum gettemp
    {
        GETTEMP_OK,
        GETTEMP_INVALID_INTERFACE,
        GETTEMP_ERROR
    };

    struct interface
    {
        bool operator==(const char *d) const { return !strcmp(d, desc); }
        bool operator==(mod::strtool &d) const { return d == desc; }
        bool operator()(temps &t) const { return gettemp(t); }

        const char *desc;
        bool (*gettemp)(temps &t);
    };

    extern interface INTERFACES[];
    extern const char *const DEFAULTINTERFACE;
    extern int NINTERFACES;
}
