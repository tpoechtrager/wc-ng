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

#include <cstdlib>
#include <queue>
#include <libg15render.h>
#include <g15daemon_client.h>

static int nextline = 0;

struct text
{
    char buf[512];
    int line;
    int x, y;
    text() : line(++nextline), x(), y() {}
};

struct g15lcd
{
    g15canvas canvas;
    int screen;
    bool isinited;
    std::queue<text*> render;

    bool init()
    {
        g15r_initCanvas(&canvas);
        screen = new_g15_screen(G15_G15RBUF);

        isinited = screen >= 0;
        return isinited;
    }

    g15lcd() : screen(), isinited() { }
    ~g15lcd() { if (isinited) g15_close_screen(screen); }
};

#define cast g15lcd *lcd = (g15lcd*)lcdhandle; while (0)

extern "C"
{
    void* createnewlcdhandle()
    {
        return new g15lcd;
    }

    void freelcdhandle(void **lcdhandle)
    {
        if (*lcdhandle)
        {
            delete (g15lcd*)*lcdhandle;
            *lcdhandle = NULL;
        }
    }

    bool initlcdhandle(void *lcdhandle, const char *app)
    {
        cast;
        return lcd->init();
    }

    bool isdevicepresent(void *lcdhandle)
    {
        cast;
        return lcd->isinited;
    }

    void* createtexthandle(void *lcdhandle)
    {
        cast;
        return new text;
    }

    bool settext(void *lcdhandle, void *texthandle, const char *t)
    {
        cast;
        static const size_t bufsize = sizeof(((text*)NULL)->buf);
        strncpy((char*)texthandle, t, bufsize);
        ((char*)texthandle)[bufsize-1] = '\0';
        lcd->render.push((text*)texthandle);
        return true;
    }

    bool setorigin(void *lcdhandle, void *texthandle, int x, int y)
    {
        cast;
        text *t = (text*)texthandle;
        t->x = x;
        t->y = y;

        return true;
    }

    void updatedisplay(void *lcdhandle)
    {
        cast;

        g15r_clearScreen(&lcd->canvas, G15_COLOR_WHITE);

        while (!lcd->render.empty())
        {
            text *t = lcd->render.front();
            lcd->render.pop();

            g15r_G15FPrint(&lcd->canvas, t->buf, t->x, t->y, G15_TEXT_MED, G15_JUSTIFY_CENTER, G15_COLOR_WHITE, 0);
        }

        g15_send(lcd->screen, (char*)lcd->canvas.buffer, G15_BUFFER_LEN);
    }
}

#undef cast
