/***********************************************************************
 *  WC-NG G15 LCD Plugin                                               *
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

extern "C"
{
    void* createnewlcdhandle();
    void freelcdhandle(void **lcdhandle);
    bool initlcdhandle(void *lcdhandle, const char *app);
    bool isdevicepresent(void *lcdhandle);
    void* createtexthandle(void *lcdhandle);
    bool settext(void *lcdhandle, void *texthandle, const char *text);
    bool setorigin(void *lcdhandle, void *texthandle, int x, int y);
    void updatedisplay(void *lcdhandle);
}
