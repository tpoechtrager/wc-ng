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

#ifdef _WIN32
#include "hwtemp.h"
#include "GetCoreTempInfo.h"
#include "nvapi_wrapper_windows.h"

namespace hwtemp
{
    //
    // SpeedFan
    //

    #pragma pack(push, 1) /* no padding */
    struct sfdata
    {
        WORD version;
        WORD flags;
        int memsize;
        int handle;
        WORD ntemps;
        WORD nfans;
        WORD nvolts;
        int temps[32];
        int fans[32];
        int volts[32];
    };
    #pragma pack(pop) /* no padding */

    static bool speedfangettemp(temps &t)
    {
        static const char *const mapname  = "SFSharedMemory_ALM";

        bool ret = true;

        HANDLE file = (HANDLE)CreateFile(mapname, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        HANDLE filemap = (HANDLE)CreateFileMapping(file, NULL, PAGE_READWRITE, 0, sizeof(sfdata), mapname);
        sfdata *sfmem = (sfdata*)MapViewOfFile(filemap, FILE_MAP_READ, 0, 0, sizeof(sfdata));

        if (sfmem)
        {
            t.ntemperatures = min(t.MAXTEMPS, (int)sfmem->ntemps);

            loopi(t.ntemperatures)
            {
                temp &temperature = t.temperatures[i];

                formatstring(temperature.desc, "speedfan_%d", i+1);
                temperature.temperature = sfmem->temps[i]/100.0f;
            }

            UnmapViewOfFile(sfmem);
        }
        else
        {
            ret = false;
        }

        CloseHandle(filemap);
        CloseHandle(file);

        return ret && t.ntemperatures > 0;
    }

    //
    // CoreTemp
    //

    class coretemp
    {
    public:

        const CORE_TEMP_SHARED_DATA *getdata()
        {
            static CORE_TEMP_SHARED_DATA coretempdata;
            if (!h || !getcoretempinfo) return NULL;
            if (getcoretempinfo(&coretempdata)) return &coretempdata;
            return NULL;
        }

        coretemp()
        {
            h = LoadLibrary("GetCoreTempInfo.dll");

            if (!h)
            {
                host->client.erroroutf_r("couldn't load GetCoreTempInfo.dll");
                host->client.conoutf_r(CON_INFO, "On 32-bit Windows you must install \"Microsoft Visual C++ 2010 Redistributable Package (x86)\" "
                                                 "before you can use this feature.");
            }

            getcoretempinfo = (fpgetcoretempinfo)GetProcAddress(h, "fnGetCoreTempInfoAlt");

            if (!getcoretempinfo)
                host->client.erroroutf_r("couldn't get address of fnGetCoreTempInfoAlt");
        }

        ~coretemp()
        {
            if (!h) return;
            FreeLibrary(h);
        }

    private:
        HMODULE h;
        bool (WINAPI *getcoretempinfo)(CORE_TEMP_SHARED_DATA *data);
        typedef bool (WINAPI *fpgetcoretempinfo)(CORE_TEMP_SHARED_DATA *data);
    } *coretemp;

    static bool coretempgettemp(temps &t)
    {
        if (!coretemp)
        {
            coretemp = new class coretemp;
            atexit(+[](){ delete coretemp; });
        }

        const CORE_TEMP_SHARED_DATA *ctdata = coretemp->getdata();

        if (ctdata)
        {
            loopi(ctdata->uiCPUCnt)
            {
                loopj(ctdata->uiCoreCnt)
                {
                    if (t.ntemperatures >= t.MAXTEMPS)
                        goto end;

                    temp &tt = t.temperatures[t.ntemperatures];

                    formatstring(tt.desc, "core %d", j+1);
                    tt.temperature = ctdata->fTemp[t.ntemperatures];
                    tt.load = (float)ctdata->uiLoad[t.ntemperatures];

                    if (ctdata->ucFahrenheit)
                        tt.temperature = (tt.temperature-32.0f)*5.0f/9.0f;

                    t.ntemperatures++;
                }
            }

            end:;
            return true;
        }

        return false;
    }

    //
    // NVIDIA
    //

    static nvsdkwrapper nvsdk;

    static bool nvgpugettemp(temps &t)
    {
        int nvtemps[64];
        int count;

        if (!(count = nvsdk.gettemp((int*)&nvtemps, sizeofarray(nvtemps))))
            return false;

        for (int i=0; i<count && i<t.MAXTEMPS; i++)
        {
            temp &tt = t.temperatures[t.ntemperatures];

            formatstring(tt.desc, "GPU %d", t.ntemperatures);
            tt.temperature = (float)nvtemps[i];

            t.ntemperatures++;
        }

        return true;
    }

    interface INTERFACES[] =
    {
        { "speedfan", speedfangettemp },
        { "coretemp", coretempgettemp },
        { "nvgpu",    nvgpugettemp    },
        { "generic",  coretempgettemp },
    };

    int NINTERFACES = sizeofarray(INTERFACES);
}
#endif //_WIN32
