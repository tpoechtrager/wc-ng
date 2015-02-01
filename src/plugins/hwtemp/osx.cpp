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

#ifdef __APPLE__
#include "hwtemp.h"
#include "smc.h"

namespace hwtemp
{
    struct initsmc
    {
        initsmc() : ok(SMCOpen() == kIOReturnSuccess) {}
        ~initsmc() { if (ok) SMCClose(); }
        bool ok;
    } smc;
  
    static bool smcgettemp(const char *hwdesc, const char *dev, temps &t)
    {
        if (!smc.ok || t.ntemperatures >= t.MAXTEMPS)
            return false;

        temp &tt = t.temperatures[t.ntemperatures];

        copystring(tt.desc, hwdesc);
        tt.temperature = SMCGetTemperature(dev);

        if (tt.temperature <= 0.0f)
            return false;

        t.ntemperatures++;
        return true;
    }

    #define SMCGETTEMPFUN(name, hwdesc, device) \
        static bool name(temps &t) { return smcgettemp(hwdesc, device, t); }

    //
    // CPU Temperature
    //

    SMCGETTEMPFUN(cpugettemp, "CPU Temperature", SMC_KEY_CPU_HEATSINK_TEMP)

    //
    // GPU Temperature
    //

    SMCGETTEMPFUN(gpugettemp, "GPU Temperature", SMC_KEY_GPU_HEATSINK_TEMP)


    interface INTERFACES[] =
    {
        { "nvgpu",      gpugettemp },
        { "gpu",        gpugettemp },
        { "cpu",        cpugettemp },
        { "generic",    cpugettemp }
    };

    int NINTERFACES = sizeofarray(INTERFACES);
}
#endif //__APPLE__
