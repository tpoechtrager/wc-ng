/***********************************************************************
 *  WC-NG - Cube 2: Sauerbraten Modification                           *
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

namespace gamemod
{
    static inline int rescalewidth(int v, int w)
    {
        static int o = 1024;
        if (w == o) return v;
        float tmp = v; tmp /= o; tmp *= w;
        return tmp;
    }

    static inline int rescaleheight(int v, int h)
    {
        static int o = 640;
        if (h == o) return v;
        float tmp = v; tmp /= o; tmp *= h;
        return tmp;
    }

    #define LINEOFFSET(x) ((x*FONTH)/2)
    #define DRAWTEXT(_fmt, left, top, alpha, ...) \
    do \
    { \
        char sbuf[512]; \
        mod::strtool buf(sbuf, sizeof(sbuf)); \
        buf.fmt(_fmt, ##__VA_ARGS__); \
        draw_text(buf.str(), left, top, 0xFF, 0xFF, 0xFF, alpha); \
    } while (0)

    class draw
    {
    public:
        template<class T> mod::strtool &operator<<(T t) { buf << t; return buf; }
        void setcolour(int red, int green, int blue) { c.r = red; c.g = green; c.b = blue; }
        void setalpha(int alpha) { this->alpha = alpha; }
        void drawtext(int left, int top) { draw_text(buf.str(), left, top, c.r, c.g, c.b, alpha); }
        void clear() { c.reset(); buf.clear(); }
        void cleartext() { buf.clear(); }
        draw() : alpha(0xFF), buf(sbuf, sizeof(sbuf)) {}
    private:
        struct rgb
        {
            int r, g, b;
            void reset() { r = g = b = 0xFF; }
            rgb() { reset(); }
        } c;
        int alpha;
        char sbuf[512];
        mod::strtool buf;
    };

    static const int COLOR_NORMAL = 0xFFFFDD;
    static const int COLOR_MASTER = 0x40FF80;
    static const int COLOR_AUTH   = 0xC040C0;
    static const int COLOR_ADMIN  = 0xFF8000;

    // playerdisplay.cpp
    void renderplayerdisplay(int conw, int conh, int FONTH, int w, int h);

    // hwtempdisplay.cpp
    void writetempnames(stream *f);
    void renderhwdisplay(int conw, int conh, int FONTH, int w, int h);

    // gamemod.cpp
    void followactioncn();
    const char *calcteamkills(void *d, char *buf, size_t len, bool scoreboard = true, bool increase = false);
    const char *countteamkills(void *a, bool increase, char *buf, size_t len);
    const char *getgamestate(void *d, const char *fmt, char *buf, int len);
    int guiplayerbgcolor(uint32_t ip, const ENetAddress &serveraddr, void *client = NULL);
    int guiplayerbgcolor(void *client, const ENetAddress &serveraddr);
    int getgamemodenum(const char *gamemode);
    bool validprotocolversion(int num);
}
