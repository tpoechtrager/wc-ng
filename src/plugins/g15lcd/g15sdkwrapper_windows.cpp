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

//
// this library needs to be compiled with MSVC
//

#include <windows.h>
#include "lglcd.h"
#include "EZ_LCD.h"

#define cast CEzLcd *lcd = (CEzLcd*)lcdhandle; while (0)

extern "C"
{
    __declspec (dllexport) void* createnewlcdhandle()
    {
        return new CEzLcd;
    }

    __declspec (dllexport) void freelcdhandle(void **lcdhandle)
    {
        if (*lcdhandle)
        {
            delete (CEzLcd*)*lcdhandle;
            *lcdhandle = NULL;
        }
    }

    __declspec (dllexport) bool initlcdhandle(void *lcdhandle, const char *app)
    {
        cast;
        return lcd->Initialize(app, LG_MONOCHROME_MODE_ONLY, FALSE, FALSE) == S_OK;
    }

    __declspec (dllexport) bool isdevicepresent(void *lcdhandle)
    {
        cast;
        return lcd->AnyDeviceOfThisFamilyPresent(LGLCD_DEVICE_FAMILY_ALL_BW_160x43) == TRUE;
    }

    __declspec (dllexport) void* createtexthandle(void *lcdhandle)
    {
        cast;
        return lcd->AddText(LG_SCROLLING_TEXT, LG_MEDIUM, DT_CENTER, 160, 1, FW_MEDIUM);
    }

    __declspec (dllexport) bool settext(void *lcdhandle, void *texthandle, const char *text)
    {
        cast;
        return lcd->SetText(texthandle, text, FALSE) == S_OK;
    }

    __declspec (dllexport) bool setorigin(void *lcdhandle, void *texthandle, int x, int y)
    {
        cast;
        return lcd->SetOrigin(texthandle, x, y) == S_OK;
    }

    __declspec (dllexport) void updatedisplay(void *lcdhandle)
    {
        cast;
        lcd->Update();
    }
}

#undef cast
