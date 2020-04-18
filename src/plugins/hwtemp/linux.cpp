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

#ifdef __linux__
#include "hwtemp.h"
#include <sensors/sensors.h>

using namespace mod::plugin;

namespace hwtemp
{
    struct initlibsensors
    {
        initlibsensors() : ok(!sensors_init(NULL)) {}
        ~initlibsensors() { if (ok) sensors_cleanup(); }
        bool ok;
    };

    static bool lmsensorsgettemp(temps &t)
    {
        static initlibsensors init;

        if (!init.ok)
            return false;

        static const char *const unknown = "unknown";
        const sensors_chip_name *chipname;
        string sensorname;
        int c = 0;

        while ((chipname = sensors_get_detected_chips(0, &c)))
        {
            const sensors_feature *feature;
            int f = 0;

            if (sensors_snprintf_chip_name(sensorname, sizeof(sensorname), chipname) < 0)
                copystring(sensorname, unknown);

            while ((feature = sensors_get_features(chipname, &f)))
            {
                if (!(feature->type & SENSORS_FEATURE_TEMP))
                    continue;

                const sensors_subfeature *subfeature;
                int s = 0;

                if ((subfeature = sensors_get_all_subfeatures(chipname, feature, &s)))
                {
                    double temperature;

                    char *devicename = sensors_get_label(chipname, feature);
                    if (!devicename) devicename = (char*)unknown;

                    if ((subfeature->flags & SENSORS_MODE_R))
                    {
                        if (!sensors_get_value(chipname, subfeature->number, &temperature))
                        {
                            if (t.ntemperatures >= t.MAXTEMPS)
                            {
                                if (devicename != unknown)
                                    free(devicename);

                                goto end;
                            }

                            temp &tt = t.temperatures[t.ntemperatures];

                            if (t.exists(devicename)) formatstring(tt.desc, "%s@%s", devicename, sensorname);
                            else copystring(tt.desc, devicename);

                            tt.temperature = (float)temperature;

                            t.ntemperatures++;
                        }
                    }

                    if (devicename != unknown)
                        free(devicename);
                }
            }
        }

        end:;
        return true;
    }

    static inline bool checkfmt(const char *fmt)
    {
        static const char expected[] = "dc"; // int, char

        size_t c = 0;

        while (*fmt++)
        {
            if (*fmt == '%' && *++fmt != '*')
            {
                 if (c >= STRLEN(expected) || expected[c++] != *fmt)
                    return false;
            }
        }

        return true;
    }

    static bool gettempfromcommand(const char *hwdesc, const char *command, const char *fmt, temps &t)
    {
        checkfmt(fmt);

        #define RETURN(v) \
        do { \
            if (strs) delete[] strs; \
            return (v); \
        } while (0)

        char buf[2048];

        if (host->command.run(command, buf, sizeof(buf)) == mod::RUNCOMAND_ERROR)
            return -1;

        mod::strtool tmp(buf, sizeof(buf), false);
        mod::strtool *strs;
        size_t n = tmp.split("\n", &strs);
        int temperature;
        char type;

        loopi(n)
        {
            mod::strtool &str = strs[i];
            int n;

            if ((n = sscanf(str.str(), fmt, &temperature, &type)) < 1)
                continue;

            if (n == 1)
              type = 'C';

            /*
             * attempt to filter implausible temperature values
             */

            if (temperature >= 500 || temperature <= -300)
                continue;

            if (t.ntemperatures >= t.MAXTEMPS)
                break;

            temp &tt = t.temperatures[t.ntemperatures];

            formatstring(tt.desc, "%s %d", hwdesc, t.ntemperatures);
            tt.temperature = (float)temperature;

            if (type == 'F')
                tt.temperature = (tt.temperature-32.0f)*5.0f/9.0f;
            else if (type != 'C')
                RETURN(false);

            t.ntemperatures++;
        }

        RETURN(t.ntemperatures > 0 ? true : false);
        #undef RETURN
    }

    #define GETTEMPFUN(name, hwdesc, command, fmt) \
        static bool name(temps &t) { return gettempfromcommand(hwdesc, command, fmt, t); }

    //
    // NVIDIA GPU
    //

    GETTEMPFUN(nvgpugettemp, "GPU", "nvidia-smi --query-gpu=temperature.gpu --format=csv,noheader", "%d")

    //
    // AMD GPU
    //

    static inline const char *amdgpufindcommand()
    {
        static const char *amdcommand = NULL;

        if (!amdcommand)
        {
            static const char *const amdcommands[] =
            {
                "amdconfig", "aticonfig"
            };

            mod::strtool path;
            for (int i=0; i<(int)sizeofarray(amdcommands); i++)
            {
                const char *command = amdcommands[i];
                path.clear();

                if (host->command.getpath(command, path))
                {
                    amdcommand = command;
                    break;
                }
            }
        }

        return amdcommand;
    }

    int amdgpugetgpucount()
    {
        static int numatigpus = -1;

        if (numatigpus < 0)
        {
            mod::strtool command;
            char buf[2048];

            const char *amdcommand = amdgpufindcommand();

            if (amdcommand)
            {
                command << amdcommand << " --lsa"; // amdconfig --lsa

                if (host->command.run(command.str(), buf, sizeof(buf)) == mod::RUNCOMAND_ERROR)
                    return -1;

                mod::strtool tmp(buf, sizeof(buf), false);
                mod::strtool *strs;
                size_t n = tmp.split("\n", &strs);

                loopi(n)
                {
                    mod::strtool &str = strs[i];
                    int num;

                    const char *p = str.str();

                    if (*p == '*') p++;
                    while (*p == ' ') p++;

                    if (sscanf(p, "%d. ", &num) != 1)
                        continue;

                    if (num > numatigpus)
                        numatigpus = num;
                }

                numatigpus++;
            }
        }

        return numatigpus;
    }

    bool amdgpugettemp(temps &t)
    {
        int gpucount = amdgpugetgpucount();

        if (gpucount <= 0)
            return false;

        static const char *const fmt = "\t\t\tSensor %*d: Temperature - %d.%*d %c";
        checkfmt(fmt);

        static bool first = true;

        static string buf;
        static mod::strtool command(buf, sizeof(buf));

        if (first)
        {
            const char *amdcommand = amdgpufindcommand();

            if (amdcommand)
                command << amdcommand << " --odgt --adapter=";

            first = false;
        }

        loopi(gpucount)
        {
            if (i) command.popuntil('=');
            command << i;

            if (!gettempfromcommand("GPU", command.str(), fmt, t))
            {
                command.popuntil('=');
                return false;
            }
        }

        command.popuntil('=');

        return true;
    }

    interface INTERFACES[] =
    {
        { "lm-sensors", lmsensorsgettemp },
        { "nvgpu",      nvgpugettemp     },
        { "amdgpu",     amdgpugettemp    },
        { "atigpu",     amdgpugettemp    },
        { "generic",    lmsensorsgettemp }
    };

    int NINTERFACES = sizeofarray(INTERFACES);
}
#endif //__linux__
