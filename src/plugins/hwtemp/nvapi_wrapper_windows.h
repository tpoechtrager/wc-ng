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

#ifdef BUILD_DLL
#define NVSDKWAPI extern "C" __declspec(dllexport)
#else
#define NVSDKWAPI extern "C" __declspec(dllimport)
#endif

#if defined(__cplusplus) && !defined(BUILD_DLL)
class nvsdkwrapper
{
public:
    nvsdkwrapper()
    {
        h = LoadLibrary("nvsdk.dll");
        ok = (h != NULL);
    }
    ~nvsdkwrapper()
    {
        if (isok())
            FreeLibrary(h);
    }

    int gettemp(int *temps, int max)
    {
        static gettempfcptr realgettemp = NULL;

        if (!ok)
            return 0;

        if (!realgettemp)
        {
            realgettemp = (gettempfcptr)GetProcAddress(h, "gettemp");

            if (!realgettemp)
                return 0;
        }

        return realgettemp(temps, max);
    }

    bool isok() { return ok; }
private:
    typedef int (*gettempfcptr)(int *temps, int max);
    HMODULE h;
    bool ok;
};
#endif //C++ && !BUILD_DLL

NVSDKWAPI int gettemp(int *temps, int max);
