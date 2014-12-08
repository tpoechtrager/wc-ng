/***********************************************************************
 *  WC-NG HWTEMP Plugin                                                *
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

#include "nvapi.h"
#include "nvapi_wrapper_windows.h"

static struct nvidia
{
    bool init()
    {
        if (isinitialized) return true;
        isinitialized = (NvAPI_Initialize() == NVAPI_OK);
        return isinitialized;
    }

    void deinit()
    {
        if (!isinitialized) return;
        NvAPI_Unload();
    }

    nvidia() : isinitialized(false) {}
    ~nvidia() { deinit(); }

    bool isinitialized;
} nvidia;

NVSDKWAPI int gettemp(int *temps, int max)
{
    NvU32 gpucount, i;
    NvPhysicalGpuHandle gpuhandles[NVAPI_MAX_PHYSICAL_GPUS];
    int *tp = temps;

    if (!nvidia.init())
        return 0;

    if (NvAPI_EnumPhysicalGPUs(gpuhandles, &gpucount) != NVAPI_OK)
        return 0;

    for (i=0; i<gpucount && i<(NvU32)max; i++)
    {
        NV_GPU_THERMAL_SETTINGS t;
        t.version = NV_GPU_THERMAL_SETTINGS_VER;
        t.count = NVAPI_MAX_THERMAL_SENSORS_PER_GPU;

        if (NvAPI_GPU_GetThermalSettings(gpuhandles[i], NVAPI_THERMAL_TARGET_NONE, &t) != NVAPI_OK)
            return 0;

        *tp++ = t.sensor[0].currentTemp;
    }

    return ((char*)tp-(char*)temps)/sizeof(*temps);
}
