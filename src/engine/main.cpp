// main.cpp: initialisation & main loop

#include "engine.h"

#ifndef _MSC_VER
#include <sched.h>
#endif

#ifdef __APPLE__
#include <CoreServices/CoreServices.h>
#include <mach/mach.h>
#include <mach/mach_time.h>
#endif

extern void cleargamma();

void cleanup()
{
    holdscreenlock;
    recorder::stop();
    cleanupserver();
    if(screen) SDL_SetWindowGrab(screen, SDL_FALSE);
    SDL_SetRelativeMouseMode(SDL_FALSE);
    SDL_ShowCursor(SDL_TRUE);
    cleargamma();
    freeocta(worldroot);
    extern void clear_command(); clear_command();
    extern void clear_console(); clear_console();
    extern void clear_mdls();    clear_mdls();
    extern void clear_sound();   clear_sound();
    closelogfile();
    SDL_Quit();
}

void quit(int *i = NULL)                     // normal exit     //NEW added int i
{
    extern void writeinitcfg();
    mod::shutdown(); //NEW
    writeinitcfg();
    writeservercfg();
    abortconnect();
    disconnect();
    localdisconnect();
    if(!i || *i == EXIT_SUCCESS) //NEW
        writecfg();
    cleanup();
    mod::plugin::unloadplugins(true); //NEW
    exit(i ? *i : EXIT_SUCCESS); //NEW  i ? *i
}

void fatal(const char *s, ...)    // failure exit
{
    static int errors = 0;
    errors++;

    if(errors <= 2) // print up to one extra recursive error
    {
        //defvformatstring(msg,s,s); //NEW commented
        //NEW
        char msg[2048];
        va_list vargs;
        va_start(vargs, s);
        vsnprintf(msg, sizeof(msg), s, vargs);
        va_end(vargs);
        //NEW END

        logoutf("%s", msg);

        if(errors <= 1) // avoid recursion
        {
            if(SDL_WasInit(SDL_INIT_VIDEO))
            {
                if(screen) SDL_SetWindowGrab(screen, SDL_FALSE);
                SDL_SetRelativeMouseMode(SDL_FALSE);
                SDL_ShowCursor(SDL_TRUE);
                cleargamma();
            }
            #ifdef WIN32
                MessageBox(NULL, msg, "Cube 2: Sauerbraten fatal error", MB_OK|MB_SYSTEMMODAL);
            #endif
            SDL_Quit();
        }
    }

    _exit(EXIT_FAILURE); //NEW exit -> _exit (do not run destructors)
}

SDL_Window *screen = NULL;
int screenw = 0, screenh = 0, desktopw = 0, desktoph = 0;
SDL_GLContext glcontext = NULL;

int lastmillis = 1, totalmillis = 1;

dynent *player = NULL;

int initing = NOT_INITING;

bool initwarning(const char *desc, int level, int type)
{
    if(initing < level) 
    {
        addchange(desc, type);
        return true;
    }
    return false;
}

#define SCR_MINW 320
#define SCR_MINH 200
#define SCR_MAXW 10000
#define SCR_MAXH 10000
#define SCR_DEFAULTW 1024
#define SCR_DEFAULTH 768
VARF(scr_w, SCR_MINW, -1, SCR_MAXW, initwarning("screen resolution"));
VARF(scr_h, SCR_MINH, -1, SCR_MAXH, initwarning("screen resolution"));
VAR(colorbits, 0, 0, 32);
VARF(depthbits, 0, 0, 32, initwarning("depth-buffer precision"));
VARF(stencilbits, 0, 0, 32, initwarning("stencil-buffer precision"));
VARF(fsaa, -1, -1, 16, initwarning("anti-aliasing"));
extern void updatevsync();
VARF(vsync, 0, 0, 1, updatevsync());
MODVARFP(vsynctear, 0, 0, 1, if(vsync) updatevsync());

void writeinitcfg()
{
    stream *f = openutf8file("init.cfg", "w");
    if(!f) return;
    f->printf("// automatically written on exit, DO NOT MODIFY\n// modify settings in game\n");
    extern int fullscreen;
    f->printf("fullscreen %d\n", fullscreen);
    f->printf("scr_w %d\n", scr_w);
    f->printf("scr_h %d\n", scr_h);
    f->printf("colorbits %d\n", colorbits);
    f->printf("depthbits %d\n", depthbits);
    f->printf("stencilbits %d\n", stencilbits);
    f->printf("fsaa %d\n", fsaa);
    f->printf("vsync %d\n", vsync);
    extern int useshaders, shaderprecision, forceglsl;
    f->printf("shaders %d\n", useshaders);
    f->printf("shaderprecision %d\n", shaderprecision);
    f->printf("forceglsl %d\n", forceglsl);
    extern int soundchans, soundfreq, soundbufferlen;
    f->printf("soundchans %d\n", soundchans);
    f->printf("soundfreq %d\n", soundfreq);
    f->printf("soundbufferlen %d\n", soundbufferlen);
    delete f;
}

COMMAND(quit, "i"); //NEW i

static void getbackgroundres(int &w, int &h)
{
    float wk = 1, hk = 1;
    if(w < 1024) wk = 1024.0f/w;
    if(h < 768) hk = 768.0f/h;
    wk = hk = max(wk, hk);
    w = int(ceil(w*wk));
    h = int(ceil(h*hk));
}

string backgroundcaption = "";
Texture *backgroundmapshot = NULL;
string backgroundmapname = "";
char *backgroundmapinfo = NULL;

void restorebackground()
{
    if(renderedframe) return;
    renderbackground(backgroundcaption[0] ? backgroundcaption : NULL, backgroundmapshot, backgroundmapname[0] ? backgroundmapname : NULL, backgroundmapinfo, true);
}

void renderbackground(const char *caption, Texture *mapshot, const char *mapname, const char *mapinfo, bool restore, bool force)
{
    if(!inbetweenframes && !force) return;

    stopsounds(); // stop sounds while loading
 
    int w = screenw, h = screenh;
    if(forceaspect) w = int(ceil(h*forceaspect));
    getbackgroundres(w, h);
    gettextres(w, h);

    holdscreenlock;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, w, h, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    defaultshader->set();
    glEnable(GL_TEXTURE_2D);

    static int lastupdate = -1, lastw = -1, lasth = -1;
    static float backgroundu = 0, backgroundv = 0, detailu = 0, detailv = 0;
    static int numdecals = 0;
    static struct decal { float x, y, size; int side; } decals[12];
    if((renderedframe && !mainmenu && lastupdate != lastmillis) || lastw != w || lasth != h)
    {
        lastupdate = lastmillis;
        lastw = w;
        lasth = h;

        backgroundu = rndscale(1);
        backgroundv = rndscale(1);
        detailu = rndscale(1);
        detailv = rndscale(1);
        numdecals = sizeof(decals)/sizeof(decals[0]);
        numdecals = numdecals/3 + rnd((numdecals*2)/3 + 1);
        float maxsize = min(w, h)/16.0f;
        loopi(numdecals)
        {
            decal d = { rndscale(w), rndscale(h), maxsize/2 + rndscale(maxsize/2), rnd(2) };
            decals[i] = d;
        }
    }
    else if(lastupdate != lastmillis) lastupdate = lastmillis;

    loopi(restore ? 1 : 3)
    {
        glColor3f(1, 1, 1);
        settexture("data/background.png", 0);
        float bu = w*0.67f/256.0f + backgroundu, bv = h*0.67f/256.0f + backgroundv;
        glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2f(0,  0);  glVertex2f(0, 0);
        glTexCoord2f(bu, 0);  glVertex2f(w, 0);
        glTexCoord2f(0,  bv); glVertex2f(0, h);
        glTexCoord2f(bu, bv); glVertex2f(w, h);
        glEnd();
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_BLEND);
        settexture("data/background_detail.png", 0);
        float du = w*0.8f/512.0f + detailu, dv = h*0.8f/512.0f + detailv;
        glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2f(0,  0);  glVertex2f(0, 0);
        glTexCoord2f(du, 0);  glVertex2f(w, 0);
        glTexCoord2f(0,  dv); glVertex2f(0, h);
        glTexCoord2f(du, dv); glVertex2f(w, h);
        glEnd();
        settexture("data/background_decal.png", 3);
        glBegin(GL_QUADS);
        loopj(numdecals)
        {
            float hsz = decals[j].size, hx = clamp(decals[j].x, hsz, w-hsz), hy = clamp(decals[j].y, hsz, h-hsz), side = decals[j].side;
            glTexCoord2f(side,   0); glVertex2f(hx-hsz, hy-hsz);
            glTexCoord2f(1-side, 0); glVertex2f(hx+hsz, hy-hsz);
            glTexCoord2f(1-side, 1); glVertex2f(hx+hsz, hy+hsz);
            glTexCoord2f(side,   1); glVertex2f(hx-hsz, hy+hsz);
        }
        glEnd();
        float lh = 0.5f*min(w, h), lw = lh*2,
              lx = 0.5f*(w - lw), ly = 0.5f*(h*0.5f - lh);
        settexture((maxtexsize ? min(maxtexsize, hwtexsize) : hwtexsize) >= 1024 && (screenw > 1280 || screenh > 800) ? "data/logo_1024.png" : "data/logo.png", 3);
        glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2f(0, 0); glVertex2f(lx,    ly);
        glTexCoord2f(1, 0); glVertex2f(lx+lw, ly);
        glTexCoord2f(0, 1); glVertex2f(lx,    ly+lh);
        glTexCoord2f(1, 1); glVertex2f(lx+lw, ly+lh);
        glEnd();

        if(caption)
        {
            int tw = text_width(caption);
            float tsz = 0.04f*min(w, h)/FONTH,
                  tx = 0.5f*(w - tw*tsz), ty = h - 0.075f*1.5f*min(w, h) - 1.25f*FONTH*tsz;
            glPushMatrix();
            glTranslatef(tx, ty, 0);
            glScalef(tsz, tsz, 1);
            draw_text(caption, 0, 0);
            glPopMatrix();
        }
        if(mapshot || mapname)
        {
            int infowidth = 12*FONTH;
            float sz = 0.35f*min(w, h), msz = (0.75f*min(w, h) - sz)/(infowidth + FONTH), x = 0.5f*(w-sz), y = ly+lh - sz/15;
            if(mapinfo)
            {
                int mw, mh;
                text_bounds(mapinfo, mw, mh, infowidth);
                x -= 0.5f*(mw*msz + FONTH*msz);
            }
            if(mapshot && mapshot!=notexture)
            {
                glBindTexture(GL_TEXTURE_2D, mapshot->id);
                glBegin(GL_TRIANGLE_STRIP);
                glTexCoord2f(0, 0); glVertex2f(x,    y);
                glTexCoord2f(1, 0); glVertex2f(x+sz, y);
                glTexCoord2f(0, 1); glVertex2f(x,    y+sz);
                glTexCoord2f(1, 1); glVertex2f(x+sz, y+sz);
                glEnd();
            }
            else
            {
                int qw, qh;
                text_bounds("?", qw, qh);
                float qsz = sz*0.5f/max(qw, qh);
                glPushMatrix();
                glTranslatef(x + 0.5f*(sz - qw*qsz), y + 0.5f*(sz - qh*qsz), 0);
                glScalef(qsz, qsz, 1);
                draw_text("?", 0, 0);
                glPopMatrix();
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            }        
            settexture("data/mapshot_frame.png", 3);
            glBegin(GL_TRIANGLE_STRIP);
            glTexCoord2f(0, 0); glVertex2f(x,    y);
            glTexCoord2f(1, 0); glVertex2f(x+sz, y);
            glTexCoord2f(0, 1); glVertex2f(x,    y+sz);
            glTexCoord2f(1, 1); glVertex2f(x+sz, y+sz);
            glEnd();
            if(mapname)
            {
                int tw = text_width(mapname);
                float tsz = sz/(8*FONTH),
                      tx = 0.9f*sz - tw*tsz, ty = 0.9f*sz - FONTH*tsz;
                if(tx < 0.1f*sz) { tsz = 0.1f*sz/tw; tx = 0.1f; }
                glPushMatrix();
                glTranslatef(x+tx, y+ty, 0);
                glScalef(tsz, tsz, 1);
                draw_text(mapname, 0, 0);
                glPopMatrix();
            }
            if(mapinfo)
            {
                glPushMatrix();
                glTranslatef(x+sz+FONTH*msz, y, 0);
                glScalef(msz, msz, 1);
                draw_text(mapinfo, 0, 0, 0xFF, 0xFF, 0xFF, 0xFF, -1, infowidth);
                glPopMatrix();
            }
        }
        glDisable(GL_BLEND);
        if(!restore) swapbuffers(false);
    }
    glDisable(GL_TEXTURE_2D);

    if(!restore)
    {
        renderedframe = false;
        copystring(backgroundcaption, caption ? caption : "");
        backgroundmapshot = mapshot;
        copystring(backgroundmapname, mapname ? mapname : "");
        if(mapinfo != backgroundmapinfo)
        {
            DELETEA(backgroundmapinfo);
            if(mapinfo) backgroundmapinfo = newstring(mapinfo);
        }
    }
}

float loadprogress = 0;

void renderprogress(float bar, const char *text, GLuint tex, bool background)   // also used during loading
{
    if(!inbetweenframes || envmapping) return;

    clientkeepalive();      // make sure our connection doesn't time out while loading maps etc.
    
    #ifdef __APPLE__
    interceptkey(SDLK_UNKNOWN); // keep the event queue awake to avoid 'beachball' cursor
    #endif

    static ullong lastprogress = 0;
    ullong now = mod::getmicroseconds();
    if(now - lastprogress <= 1000000/59) return;
    lastprogress = now;

    extern int sdl_backingstore_bug;
    if(background || sdl_backingstore_bug > 0) restorebackground();

    int w = screenw, h = screenh;
    if(forceaspect) w = int(ceil(h*forceaspect));
    getbackgroundres(w, h);
    gettextres(w, h);

    holdscreenlock;
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, w, h, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glEnable(GL_TEXTURE_2D);
    defaultshader->set();
    glColor3f(1, 1, 1);

    float fh = 0.075f*min(w, h), fw = fh*10,
          fx = renderedframe ? w - fw - fh/4 : 0.5f*(w - fw), 
          fy = renderedframe ? fh/4 : h - fh*1.5f,
          fu1 = 0/512.0f, fu2 = 511/512.0f,
          fv1 = 0/64.0f, fv2 = 52/64.0f;
    settexture("data/loading_frame.png", 3);
    glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(fu1, fv1); glVertex2f(fx,    fy);
    glTexCoord2f(fu2, fv1); glVertex2f(fx+fw, fy);
    glTexCoord2f(fu1, fv2); glVertex2f(fx,    fy+fh);
    glTexCoord2f(fu2, fv2); glVertex2f(fx+fw, fy+fh);
    glEnd();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float bw = fw*(511 - 2*17)/511.0f, bh = fh*20/52.0f,
          bx = fx + fw*17/511.0f, by = fy + fh*16/52.0f,
          bv1 = 0/32.0f, bv2 = 20/32.0f,
          su1 = 0/32.0f, su2 = 7/32.0f, sw = fw*7/511.0f,
          eu1 = 23/32.0f, eu2 = 30/32.0f, ew = fw*7/511.0f,
          mw = bw - sw - ew,
          ex = bx+sw + max(mw*bar, fw*7/511.0f);
    if(bar > 0)
    {
        settexture("data/loading_bar.png", 3);
        glBegin(GL_QUADS);
        glTexCoord2f(su1, bv1); glVertex2f(bx,    by);
        glTexCoord2f(su2, bv1); glVertex2f(bx+sw, by);
        glTexCoord2f(su2, bv2); glVertex2f(bx+sw, by+bh);
        glTexCoord2f(su1, bv2); glVertex2f(bx,    by+bh);

        glTexCoord2f(su2, bv1); glVertex2f(bx+sw, by);
        glTexCoord2f(eu1, bv1); glVertex2f(ex,    by);
        glTexCoord2f(eu1, bv2); glVertex2f(ex,    by+bh);
        glTexCoord2f(su2, bv2); glVertex2f(bx+sw, by+bh);

        glTexCoord2f(eu1, bv1); glVertex2f(ex,    by);
        glTexCoord2f(eu2, bv1); glVertex2f(ex+ew, by);
        glTexCoord2f(eu2, bv2); glVertex2f(ex+ew, by+bh);
        glTexCoord2f(eu1, bv2); glVertex2f(ex,    by+bh);
        glEnd();
    }

    if(text)
    {
        int tw = text_width(text);
        float tsz = bh*0.8f/FONTH;
        if(tw*tsz > mw) tsz = mw/tw;
        glPushMatrix();
        glTranslatef(bx+sw, by + (bh - FONTH*tsz)/2, 0);
        glScalef(tsz, tsz, 1);
        draw_text(text, 0, 0);
        glPopMatrix();
    }

    glDisable(GL_BLEND);

    if(tex)
    {
        glBindTexture(GL_TEXTURE_2D, tex);
        float sz = 0.35f*min(w, h), x = 0.5f*(w-sz), y = 0.5f*min(w, h) - sz/15;
        glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2f(0, 0); glVertex2f(x,    y);
        glTexCoord2f(1, 0); glVertex2f(x+sz, y);
        glTexCoord2f(0, 1); glVertex2f(x,    y+sz);
        glTexCoord2f(1, 1); glVertex2f(x+sz, y+sz);
        glEnd();

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        settexture("data/mapshot_frame.png", 3);
        glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2f(0, 0); glVertex2f(x,    y);
        glTexCoord2f(1, 0); glVertex2f(x+sz, y);
        glTexCoord2f(0, 1); glVertex2f(x,    y+sz);
        glTexCoord2f(1, 1); glVertex2f(x+sz, y+sz);
        glEnd();
        glDisable(GL_BLEND);
    }

    glDisable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    swapbuffers(false);
}

MODVARNP(relativemouse, userelativemouse, 0, 1, 1);

bool shouldgrab = false, grabinput = false, minimized = false, canrelativemouse = true, relativemouse = false;
int keyrepeatmask = 0, textinputmask = 0;

void keyrepeat(bool on, int mask)
{
    if(on) keyrepeatmask |= mask;
    else keyrepeatmask &= ~mask;
}

void textinput(bool on, int mask)
{
    if(on) 
    {
        if(!textinputmask) SDL_StartTextInput(); 
        textinputmask |= mask;
    }
    else
    {
        textinputmask &= ~mask;
        if(!textinputmask) SDL_StopTextInput();
    }
}

void inputgrab(bool on)
{
    if(on)
    {
        SDL_ShowCursor(SDL_FALSE);
        if(canrelativemouse && userelativemouse)
        {
            if(SDL_SetRelativeMouseMode(SDL_TRUE) >= 0) 
            {
                SDL_SetWindowGrab(screen, SDL_TRUE);
                relativemouse = true;
            }
            else 
            {
                SDL_SetWindowGrab(screen, SDL_FALSE);
                canrelativemouse = false;
                relativemouse = false;
            }
        }
    }
    else 
    {
        SDL_ShowCursor(SDL_TRUE);
        if(relativemouse)
        {
            SDL_SetWindowGrab(screen, SDL_FALSE);
            SDL_SetRelativeMouseMode(SDL_FALSE);
            relativemouse = false;
        }
    }
    shouldgrab = false;
}

bool initwindowpos = false;

void setfullscreen(bool enable)
{
    holdscreenlock;
    if(!screen) return;
    //initwarning(enable ? "fullscreen" : "windowed");
    SDL_SetWindowFullscreen(screen, enable ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
    if(!enable) 
    {
        SDL_SetWindowSize(screen, scr_w, scr_h);
        if(initwindowpos)
        {
            int winx = SDL_WINDOWPOS_CENTERED, winy = SDL_WINDOWPOS_CENTERED;
            SDL_SetWindowPosition(screen, winx, winy);
            initwindowpos = false;
        }
    }
}

#ifdef _DEBUG
VARF(fullscreen, 0, 0, 1, setfullscreen(fullscreen!=0));
#else
VARF(fullscreen, 0, 1, 1, setfullscreen(fullscreen!=0));
#endif

void screenres(int w, int h)
{
    scr_w = clamp(w, SCR_MINW, SCR_MAXW);
    scr_h = clamp(h, SCR_MINH, SCR_MAXH);
    if(screen)
    {
        scr_w = min(scr_w, desktopw);
        scr_h = min(scr_h, desktoph);
        holdscreenlock;
        if(SDL_GetWindowFlags(screen) & SDL_WINDOW_FULLSCREEN) gl_resize();
        else SDL_SetWindowSize(screen, scr_w, scr_h);
    }
    else 
    {
        initwarning("screen resolution");
    }
}

ICOMMAND(screenres, "ii", (int *w, int *h), screenres(*w, *h));

static int curgamma = 100;
VARFP(gamma, 30, 100, 300,
{
    if(gamma == curgamma) return;
    curgamma = gamma;
    holdscreenlock;
    if(SDL_SetWindowBrightness(screen, gamma/100.0f)==-1) conoutf(CON_ERROR, "Could not set gamma: %s", SDL_GetError());
});

void restoregamma()
{
    if(curgamma == 100) return;
    holdscreenlock;
    SDL_SetWindowBrightness(screen, curgamma/100.0f);
}

void cleargamma()
{
    holdscreenlock;
    if(curgamma != 100 && screen) SDL_SetWindowBrightness(screen, 1.0f);
}

VAR(dbgmodes, 0, 0, 1);

void setupscreen(int &useddepthbits, int &usedfsaa)
{
    holdscreenlock;
    if(glcontext)
    {
        SDL_GL_DeleteContext(glcontext);
        glcontext = NULL;
    }
    if(screen)
    {
        SDL_DestroyWindow(screen);
        screen = NULL;
    }

    SDL_DisplayMode desktop;
    if(SDL_GetDesktopDisplayMode(0, &desktop) < 0) fatal("failed querying desktop display mode: %s", SDL_GetError());
    desktopw = desktop.w;
    desktoph = desktop.h;

    if(scr_h < 0) scr_h = SCR_DEFAULTH;
    if(scr_w < 0) scr_w = (scr_h*desktopw)/desktoph;
    scr_w = min(scr_w, desktopw);
    scr_h = min(scr_h, desktoph);

    int winx = SDL_WINDOWPOS_UNDEFINED, winy = SDL_WINDOWPOS_UNDEFINED, winw = scr_w, winh = scr_h, flags = SDL_WINDOW_RESIZABLE;
    if(fullscreen)
    {
        winw = desktopw;
        winh = desktoph;
        flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
        initwindowpos = true;
    }

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    static int configs[] =
    {
        0x7, /* try everything */
        0x6, 0x5, 0x3, /* try disabling one at a time */
        0x4, 0x2, 0x1, /* try disabling two at a time */
        0 /* try disabling everything */
    };
    int config = 0;
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
    if(!depthbits) SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    if(!fsaa)
    {
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
    }
    loopi(sizeof(configs)/sizeof(configs[0]))
    {
        config = configs[i];
        if(!depthbits && config&1) continue;
        if(!stencilbits && config&2) continue;
        if(fsaa<=0 && config&4) continue;
        if(depthbits) SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, config&1 ? depthbits : 16);
        if(stencilbits)
        {
            hasstencil = config&2 ? stencilbits : 0;
            SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, hasstencil);
        }
        else hasstencil = 0;
        if(fsaa>0)
        {
            SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, config&4 ? 1 : 0);
            SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, config&4 ? fsaa : 0);
        }
        screen = SDL_CreateWindow("Cube 2: Sauerbraten", winx, winy, winw, winh, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_MOUSE_FOCUS | flags);
        if(screen) break;
    }
    if(!screen) fatal("failed to create OpenGL window: %s", SDL_GetError());
    else
    {
        if(depthbits && (config&1)==0) conoutf(CON_WARN, "%d bit z-buffer not supported - disabling", depthbits);
        if(stencilbits && (config&2)==0) conoutf(CON_WARN, "Stencil buffer not supported - disabling");
        if(fsaa>0 && (config&4)==0) conoutf(CON_WARN, "%dx anti-aliasing not supported - disabling", fsaa);
    }

    SDL_SetWindowMinimumSize(screen, SCR_MINW, SCR_MINH);
    SDL_SetWindowMaximumSize(screen, SCR_MAXW, SCR_MAXH);

    glcontext = SDL_GL_CreateContext(screen);
    if(!glcontext) fatal("failed to create OpenGL context: %s", SDL_GetError());
    updatevsync();

    SDL_GetWindowSize(screen, &screenw, &screenh);

    useddepthbits = config&1 ? depthbits : 0;
    usedfsaa = config&4 ? fsaa : 0;
}

void updatevsync()
{
    if(!glcontext) return;
    holdscreenlock;
    if(!SDL_GL_SetSwapInterval(vsync ? (vsynctear ? -1 : 1) : 0)) return;
    if(vsync && vsynctear) conoutf("vsynctear not supported, or you need to restart sauer to apply changes.");
    else if(vsync) conoutf("vsynctear not supported, or you need to restart sauer to apply changes.");
    else conoutf("You need to restart sauer to disable vsync.");
}

void resetgl()
{
    holdscreenlock;
    clearchanges(CHANGE_GFX);

    renderbackground("resetting OpenGL");

    extern void cleanupva();
    extern void cleanupparticles();
    extern void cleanupsky();
    extern void cleanupmodels();
    extern void cleanuptextures();
    extern void cleanuplightmaps();
    extern void cleanupblendmap();
    extern void cleanshadowmap();
    extern void cleanreflections();
    extern void cleanupglare();
    extern void cleanupdepthfx();
    extern void cleanupshaders();
    extern void cleanupgl();
    recorder::cleanup();
    cleanupva();
    cleanupparticles();
    cleanupsky();
    cleanupmodels();
    cleanuptextures();
    cleanuplightmaps();
    cleanupblendmap();
    cleanshadowmap();
    cleanreflections();
    cleanupglare();
    cleanupdepthfx();
    cleanupshaders();
    cleanupgl();
    
    int useddepthbits = 0, usedfsaa = 0;
    setupscreen(useddepthbits, usedfsaa);

    inputgrab(grabinput);

    gl_init(useddepthbits, usedfsaa);

    extern void reloadfonts();
    extern void reloadtextures();
    extern void reloadshaders();
    inbetweenframes = false;
    if(!reloadtexture(*notexture) ||
       !reloadtexture("data/logo.png") ||
       !reloadtexture("data/logo_1024.png") || 
       !reloadtexture("data/background.png") ||
       !reloadtexture("data/background_detail.png") ||
       !reloadtexture("data/background_decal.png") ||
       !reloadtexture("data/mapshot_frame.png") ||
       !reloadtexture("data/loading_frame.png") ||
       !reloadtexture("data/loading_bar.png"))
        fatal("failed to reload core texture");
    reloadfonts();
    inbetweenframes = true;
    renderbackground("initializing...");
	restoregamma();
    reloadshaders();
    reloadtextures();
    initlights();
    allchanged(true);
}

COMMAND(resetgl, "");

vector<SDL_Event> events;

void pushevent(const SDL_Event &e)
{
    events.add(e); 
}

static bool filterevent(const SDL_Event &event)
{
    switch(event.type)
    {
        case SDL_MOUSEMOTION:
            if(grabinput && !relativemouse && !(SDL_GetWindowFlags(screen) & SDL_WINDOW_FULLSCREEN))
            {
                if(event.motion.x == screenw / 2 && event.motion.y == screenh / 2) 
                    return false;  // ignore any motion events generated by SDL_WarpMouse
                #ifdef __APPLE__
                if(event.motion.y == 0) 
                    return false;  // let mac users drag windows via the title bar
                #endif
            }
            break;
    }
    return true;
}

static inline bool pollevent(SDL_Event &event)
{
    while(SDL_PollEvent(&event))
    {
        if(filterevent(event)) return true;
    }
    return false;
}

bool interceptkey(int sym)
{
    static int lastintercept = SDLK_UNKNOWN;
    int len = lastintercept == sym ? events.length() : 0;
    SDL_Event event;
    while(pollevent(event))
    {
        switch(event.type)
        {
            case SDL_MOUSEMOTION: break;
            default: pushevent(event); break;
        }
    }
    lastintercept = sym;
    if(sym != SDLK_UNKNOWN) for(int i = len; i < events.length(); i++)
    {
        if(events[i].type == SDL_KEYDOWN && events[i].key.keysym.sym == sym) { events.remove(i); return true; }
    }
    return false;
}

static void ignoremousemotion()
{
    SDL_Event e;
    SDL_PumpEvents();
    while(SDL_PeepEvents(&e, 1, SDL_GETEVENT, SDL_MOUSEMOTION, SDL_MOUSEMOTION));
}

static void resetmousemotion()
{
    if(grabinput && !relativemouse && !(SDL_GetWindowFlags(screen) & SDL_WINDOW_FULLSCREEN))
    {
        SDL_WarpMouseInWindow(screen, screenw / 2, screenh / 2);
    }
}

static void checkmousemotion(int &dx, int &dy)
{
    loopv(events)
    {
        SDL_Event &event = events[i];
        if(event.type != SDL_MOUSEMOTION)
        { 
            if(i > 0) events.remove(0, i); 
            return; 
        }
        dx += event.motion.xrel;
        dy += event.motion.yrel;
    }
    events.setsize(0);
    SDL_Event event;
    while(pollevent(event))
    {
        if(event.type != SDL_MOUSEMOTION)
        {
            events.add(event);
            return;
        }
        dx += event.motion.xrel;
        dy += event.motion.yrel;
    }
}

void checkinput()
{
    SDL_Event event;
    //int lasttype = 0, lastbut = 0;
    bool mousemoved = false; 
    while(events.length() || pollevent(event))
    {
        if(events.length()) event = events.remove(0);

        switch(event.type)
        {
            case SDL_QUIT:
                quit();
                return;

            case SDL_TEXTINPUT:
            {
                static uchar buf[SDL_TEXTINPUTEVENT_TEXT_SIZE+1];
                int len = decodeutf8(buf, int(sizeof(buf)-1), (const uchar *)event.text.text, strlen(event.text.text));
                if(len > 0) { buf[len] = '\0'; processtextinput((const char *)buf, len); }
                break;
            }

            case SDL_KEYDOWN:
            case SDL_KEYUP:
                if(keyrepeatmask || !event.key.repeat)
                    processkey(event.key.keysym.sym, event.key.state==SDL_PRESSED);
                break;

            case SDL_WINDOWEVENT:
                switch(event.window.event)
                {
                    case SDL_WINDOWEVENT_CLOSE:
                        quit();
                        break;

                    case SDL_WINDOWEVENT_FOCUS_GAINED:
                        shouldgrab = true;
                        break;
                    case SDL_WINDOWEVENT_ENTER:
                        inputgrab(grabinput = true);
                        break;

                    case SDL_WINDOWEVENT_LEAVE:
                    case SDL_WINDOWEVENT_FOCUS_LOST:
                        inputgrab(grabinput = false);
                        break;

                    case SDL_WINDOWEVENT_MINIMIZED:
                        minimized = true;
                        break;

                    case SDL_WINDOWEVENT_MAXIMIZED:
                    case SDL_WINDOWEVENT_RESTORED:
                        minimized = false;
                        break;

                    case SDL_WINDOWEVENT_RESIZED:
                        break;

                    case SDL_WINDOWEVENT_SIZE_CHANGED:
                    {
                        holdscreenlock;
                        SDL_GetWindowSize(screen, &screenw, &screenh);
                        if(!(SDL_GetWindowFlags(screen) & SDL_WINDOW_FULLSCREEN))
                        {
                            scr_w = clamp(screenw, SCR_MINW, SCR_MAXW);
                            scr_h = clamp(screenh, SCR_MINH, SCR_MAXH);
                        }
                        gl_resize();
                    }
                        break;
                }
                break;

            case SDL_MOUSEMOTION:
                if(grabinput)
                {
                    int dx = event.motion.xrel, dy = event.motion.yrel;
                    checkmousemotion(dx, dy);
                    if(!g3d_movecursor(dx, dy)) mousemove(dx, dy);
                    mousemoved = true;
                }
                else if(shouldgrab) inputgrab(grabinput = true);
                break;

            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
#ifdef __linux__
#define keycodeshift 0
#else
#define keycodeshift 2*(event.button.button>=SDL_BUTTON_X1)
#endif
                processkey(-event.button.button - keycodeshift, event.button.state==SDL_PRESSED);
                break;
    
            case SDL_MOUSEWHEEL:
                if(event.wheel.y > 0) { processkey(-4, true); processkey(-4, false); }
                else if(event.wheel.y < 0) { processkey(-5, true); processkey(-5, false); }
                if(event.wheel.x > 0) { processkey(-35, true); processkey(-35, false); }
                else if(event.wheel.x < 0) { processkey(-36, true); processkey(-36, false); }
                break;
        }
    }
    if(mousemoved) resetmousemotion();
}

void swapbuffers(bool overlay)
{
    holdscreenlock;
    recorder::capture(overlay);
    SDL_GL_SwapWindow(screen);
}
 
VAR(menufps, 0, 60, 1000);
VARP(maxfps, 0, 200, 1000);
MODVARFP(multipoll, -1, 0, 1,
{
    drawer::keepgl(multipoll == 1);
    if(initing == NOT_INITING && multipoll < 0 && (vsync || !maxfps))
    {
        conoutf(CON_WARN, "/multipoll -1 makes sense only with /vsync 0 and /maxfps non-zero. "
                          "Make sure you really understand what /multipoll is doing.");
    }
});
MODVAR(microdelay, 0, 50, 100);

static inline void delay(llong us)
{
#ifdef _MSC_VER
    int ms = us / 1000;
    us %= 1000;
    Sleep(ms);
    if(ms)
    {
        llong begin = mod::getmicroseconds();
        while((llong)mod::getmicroseconds() - begin < us) Sleep(0);
    }
#else
    usleep(us);
#endif
}

bool limitfps(ullong &curticks)
{
    static ullong lastdraw = 0, lastrefresh = 0;
    int fpslimit = (mainmenu || minimized) && menufps ? (maxfps ? min(maxfps, menufps) : menufps) : maxfps;
    ullong nextdraw = (fpslimit ? 1000000ULL / fpslimit : 0) + lastdraw;
    bool dodraw;
    if(multipoll)
    {
        if(fpslimit && nextdraw <= curticks)
        {
            dodraw = true;
            goto frame;
        }
        dodraw = fpslimit == 0;
        ullong nextrefresh = lastrefresh + microdelay;
        if(nextrefresh <= curticks) goto frame;
        delay(nextrefresh - curticks);
        curticks = mod::getmicroseconds();
        return limitfps(curticks);
    }
    else
    {
        if(nextdraw <= curticks)
        {
            dodraw = true;
            goto frame;
        }
        delay(nextdraw - curticks);
        curticks = mod::getmicroseconds();
        return limitfps(curticks);
    }
    frame:
    lastrefresh = curticks;
    if(dodraw) lastdraw = curticks;
    return dodraw;
}


#if defined(WIN32) && !defined(_DEBUG) && !defined(__GNUC__)
//NEW
#ifdef PUBLIC_RELEASE
bool createminidump(EXCEPTION_POINTERS *ep, const char *dmpfile)
{
    bool res = false;

    HANDLE file = CreateFile(dmpfile, GENERIC_READ|GENERIC_WRITE,
                             0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if(file && file != INVALID_HANDLE_VALUE)
    {
        MINIDUMP_EXCEPTION_INFORMATION mdei;

        mdei.ThreadId = GetCurrentThreadId();
        mdei.ExceptionPointers = ep;
        mdei.ClientPointers = FALSE;

        MINIDUMP_TYPE mdt = MiniDumpNormal;

        res = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
                                file, mdt, ep? &mdei : NULL, NULL, NULL) == TRUE;
        CloseHandle(file);
    }

    return res;
}
#endif
//NEW END
void stackdumper(unsigned int type, EXCEPTION_POINTERS *ep)
{
//NEW
#ifdef PUBLIC_RELEASE
    string buf;
    mod::strtool home(buf, sizeof(buf));
    home = gethomedir();
    if(home) home += "/";
    defformatstring(dmp)("%scrash.v%s-%s.%d.dmp", home.str(), CLIENTVERSION.str().str(),
                         getwcrevision(), totalmillis);
    path(dmp);

    if (!createminidump(ep, dmp))
        copystring(dmp, "failed to create dump");

    const char *fmt =
        "The client has crashed. Please write me an email (t.poechtrager@gmail.com) "
        "including a full description of how it happened so we can fix this issue in "
        "future releases. Please attach the created minidump to the email "
        "(%s). Thanks!";

    fatal(fmt, dmp);
    return;
#endif
//NEW END
    if(!ep) fatal("unknown type");
    EXCEPTION_RECORD *er = ep->ExceptionRecord;
    CONTEXT *context = ep->ContextRecord;
    string out, t;
    formatstring(out)("Cube 2: Sauerbraten Win32 Exception: 0x%x [0x%x]\n\n", er->ExceptionCode, er->ExceptionCode==EXCEPTION_ACCESS_VIOLATION ? er->ExceptionInformation[1] : -1);
    SymInitialize(GetCurrentProcess(), NULL, TRUE);
#ifdef _AMD64_
	STACKFRAME64 sf = {{context->Rip, 0, AddrModeFlat}, {}, {context->Rbp, 0, AddrModeFlat}, {context->Rsp, 0, AddrModeFlat}, 0};
    while(::StackWalk64(IMAGE_FILE_MACHINE_AMD64, GetCurrentProcess(), GetCurrentThread(), &sf, context, NULL, ::SymFunctionTableAccess, ::SymGetModuleBase, NULL))
	{
		union { IMAGEHLP_SYMBOL64 sym; char symext[sizeof(IMAGEHLP_SYMBOL64) + sizeof(string)]; };
		sym.SizeOfStruct = sizeof(sym);
		sym.MaxNameLength = sizeof(symext) - sizeof(sym);
		IMAGEHLP_LINE64 line;
		line.SizeOfStruct = sizeof(line);
        DWORD64 symoff;
		DWORD lineoff;
        if(SymGetSymFromAddr64(GetCurrentProcess(), sf.AddrPC.Offset, &symoff, &sym) && SymGetLineFromAddr64(GetCurrentProcess(), sf.AddrPC.Offset, &lineoff, &line))
#else
    STACKFRAME sf = {{context->Eip, 0, AddrModeFlat}, {}, {context->Ebp, 0, AddrModeFlat}, {context->Esp, 0, AddrModeFlat}, 0};
    while(::StackWalk(IMAGE_FILE_MACHINE_I386, GetCurrentProcess(), GetCurrentThread(), &sf, context, NULL, ::SymFunctionTableAccess, ::SymGetModuleBase, NULL))
	{
		union { IMAGEHLP_SYMBOL sym; char symext[sizeof(IMAGEHLP_SYMBOL) + sizeof(string)]; };
		sym.SizeOfStruct = sizeof(sym);
		sym.MaxNameLength = sizeof(symext) - sizeof(sym);
		IMAGEHLP_LINE line;
		line.SizeOfStruct = sizeof(line);
        DWORD symoff, lineoff;
        if(SymGetSymFromAddr(GetCurrentProcess(), sf.AddrPC.Offset, &symoff, &sym) && SymGetLineFromAddr(GetCurrentProcess(), sf.AddrPC.Offset, &lineoff, &line))
#endif
        {
            char *del = strrchr(line.FileName, '\\');
            formatstring(t)("%s - %s [%d]\n", sym.Name, del ? del + 1 : line.FileName, line.LineNumber);
            concatstring(out, t);
        }
    }
    fatal(out);
}
#endif

int currentfps[3] = {0, 0, 0};

void updatefps(int which, int value = 1)
{
    static int fpsaccumulator[3] = { 0, 0, 0 };
    static int fpsbasemillis = 0, drawmillistot = 0;
    if(totalmillis - fpsbasemillis >= 1000)
    {
        loopi(3)
        {
            currentfps[i] = fpsaccumulator[i];
            fpsaccumulator[i] = 0;
        }
        if(drawmillistot) currentfps[2]/=drawmillistot;
        drawmillistot = 0;
        fpsbasemillis = totalmillis;
    }
    fpsaccumulator[which]+=value;
    if(which==2) drawmillistot++;
}

int getfps(int which)
{
    return currentfps[which];
}

void getfps_(int *raw, int *which)
{
    intret(getfps(clamp(*which, 0, 2)));
}

COMMANDN(getfps, getfps_, "ii");

bool inbetweenframes = false, renderedframe = true;

static bool findarg(int argc, char **argv, const char *str)
{
    for(int i = 1; i<argc; i++) if(strstr(argv[i], str)==argv[i]) return true;
    return false;
}

static int clockrealbase = 0, clockvirtbase = 0;
static void clockreset() { clockrealbase = SDL_GetTicks(); clockvirtbase = totalmillis; }
VARFP(clockerror, 990000, 1000000, 1010000, clockreset());
VARFP(clockfix, 0, 0, 1, clockreset());

int getclockmillis()
{
    int millis = SDL_GetTicks() - clockrealbase;
    if(clockfix) millis = int(millis*(double(clockerror)/1000000));
    millis += clockvirtbase;
    return max(millis, totalmillis);
}

VAR(numcpus, 1, 1, 16);

//NEW
MODVARP(benchmarkframe, 0, 0, 1);
static double totaltime = 0.0;
static uint totalcounter = 0;

#ifdef __GNUC__
__inline 
#endif
void framebenchmark(bool create = false)
{
    if(!benchmarkframe) return;
    static benchmark bench("frame", true);
    if(create && !bench.active())
    {
        if(vsync) conoutf("benchmarking frames doesn't work properly with vsync enabled");
        else bench.start();
    }
    else
    {
        if(bench.active())
        {
            bench.calc();
            double totaltimeold = totaltime;
            totaltime += bench.gettime();
            if(totaltime<totaltimeold)
            {
                totaltime = 0.0;
                totalcounter = 0;
            }
            else
            {
                totalcounter++;
                conoutf("frame took %.6lf ms | median: %.6lf ms | possible fps: %.0f", 
                    bench.gettime(), totaltime/double(totalcounter ? totalcounter : 1), 1000.0/bench.gettime());
            }
            bench.reset();
        }
    }
}

ICOMMAND(resetframebenchmark, "", (), { totaltime = 0.0; totalcounter = 0.0; });
//NEW END

#ifdef __APPLE__
#define main SDL_main
#endif

int main(int argc, char **argv)
{
    #ifdef WIN32
    //atexit((void (__cdecl *)(void))_CrtDumpMemoryLeaks);
    #ifndef _DEBUG
    #ifndef __GNUC__
    #ifndef __clang__ //NEW
    __try {
    #endif
    #endif
    #endif
    #endif

    setlogfile(NULL);

    int dedicated = 0;
    char *load = NULL, *initscript = NULL;

    initing = INIT_RESET;
    for(int i = 1; i<argc; i++)
    {
        if(argv[i][0]=='-') switch(argv[i][1])
        {
            case 'q': 
			{
				const char *dir = sethomedir(&argv[i][2]);
				if(dir) logoutf("Using home directory: %s", dir);
				break;
			}
        }
    }
    execfile("init.cfg", false);
    for(int i = 1; i<argc; i++)
    {
        if(argv[i][0]=='-') switch(argv[i][1])
        {
            case 'q': /* parsed first */ break;
            case 'r': /* compat, ignore */ break;
            case 'k':
            {
                const char *dir = addpackagedir(&argv[i][2]);
                if(dir) logoutf("Adding package directory: %s", dir);
                break;
            }
            case 'g': logoutf("Setting log file: %s", &argv[i][2]); setlogfile(&argv[i][2]); break;
            case 'd': dedicated = atoi(&argv[i][2]); if(dedicated<=0) dedicated = 2; break;
            case 'w': scr_w = clamp(atoi(&argv[i][2]), SCR_MINW, SCR_MAXW); if(!findarg(argc, argv, "-h")) scr_h = -1; break;
            case 'h': scr_h = clamp(atoi(&argv[i][2]), SCR_MINH, SCR_MAXH); if(!findarg(argc, argv, "-w")) scr_w = -1; break;
            case 'z': depthbits = atoi(&argv[i][2]); break;
            case 'b': /* compat, ignore */ break;
            case 'a': fsaa = atoi(&argv[i][2]); break;
            case 'v': vsync = atoi(&argv[i][2]); if(vsync < 0) { vsynctear = 1; vsync = 1; } else vsynctear = 0; break;
            case 't': fullscreen = atoi(&argv[i][2]); break;
            case 's': stencilbits = atoi(&argv[i][2]); break;
            case 'f': 
            {
                extern int useshaders, shaderprecision, forceglsl;
                int sh = -1, prec = shaderprecision;
                for(int j = 2; argv[i][j]; j++) switch(argv[i][j])
                {
                    case 'a': case 'A': forceglsl = 0; sh = 1; break;
                    case 'g': case 'G': forceglsl = 1; sh = 1; break;
                    case 'f': case 'F': case '0': sh = 0; break;
                    case '1': case '2': case '3': if(sh < 0) sh = 1; prec = argv[i][j] - '1'; break;
                    default: break;
                }
                useshaders = sh > 0 ? 1 : 0;
                shaderprecision = prec;
                break;
            }
            case 'l': 
            {
                char pkgdir[] = "packages/"; 
                load = strstr(path(&argv[i][2]), path(pkgdir)); 
                if(load) load += sizeof(pkgdir)-1; 
                else load = &argv[i][2]; 
                break;
            }
            case 'x': initscript = &argv[i][2]; break;
            default: if(!serveroption(argv[i])) gameargs.add(argv[i]); break;
        }
        else gameargs.add(argv[i]);
    }

    initing = NOT_INITING;

    numcpus = clamp(SDL_GetCPUCount(), 1, 16);

    if(dedicated <= 1)
    {
        logoutf("init: sdl");

        int par = 0;
        #ifdef _DEBUG
        par = SDL_INIT_NOPARACHUTE;
        #endif

        if(SDL_Init(SDL_INIT_TIMER|SDL_INIT_VIDEO|SDL_INIT_AUDIO|par)<0) fatal("Unable to initialize SDL: %s", SDL_GetError());
    }
    
    logoutf("init: net");
    if(enet_initialize()<0) fatal("Unable to initialise network module");
    atexit(enet_deinitialize);
    enet_time_set(0);

    logoutf("init: game");
    game::parseoptions(gameargs);
    initserver(dedicated>0, dedicated>1);  // never returns if dedicated
    ASSERT(dedicated <= 1);
    game::initclient();

    logoutf("init: video");
    SDL_SetHint(SDL_HINT_GRAB_KEYBOARD, "0");
    #if !defined(WIN32) && !defined(__APPLE__)
    SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");
    #endif
    int useddepthbits = 0, usedfsaa = 0;
    setupscreen(useddepthbits, usedfsaa);
    SDL_ShowCursor(SDL_FALSE);
    SDL_StopTextInput(); // workaround for spurious text-input events getting sent on first text input toggle?

    logoutf("init: gl");
    gl_checkextensions();
    gl_init(useddepthbits, usedfsaa);
    notexture = textureload("packages/textures/notexture.png");
    if(!notexture) fatal("could not find core textures");

    logoutf("init: console");
    if(!execfile("data/stdlib.cfg", false)) fatal("cannot find data files (you are running from the wrong folder, try .bat file in the main folder)");   // this is the first file we load.
    if(!execfile("data/font.cfg", false)) fatal("cannot find font definitions");
    if(!setfont("default")) fatal("no default font specified");

    inbetweenframes = true;
    renderbackground("initializing...");

    logoutf("init: gl: effects");
    loadshaders();
    particleinit();
    initdecals();

    logoutf("init: world");
    camera1 = player = game::iterdynents(0);
    emptymap(0, true, NULL, false);

    logoutf("init: sound");
    initsound();

    mod::startup(); //NEW

    logoutf("init: cfg");
    execfile("data/keymap.cfg");
    execfile("data/stdedit.cfg");
    execfile("data/menus.cfg");
    execfile("data/sounds.cfg");
    execfile("data/brush.cfg");
    execfile("mybrushes.cfg", false);
    if(game::savedservers()) execfile(game::savedservers(), false);
    
    identflags |= IDF_PERSIST;
    
    initing = INIT_LOAD;
    execfile(game::wcconfig(), false); //NEW
    if(!execfile(game::savedconfig(), false)) 
    {
        execfile(game::defaultconfig());
        writecfg(game::restoreconfig());
    }
    mod::chat::setupbinds();           //NEW
    mod::wcautocheckversion();         //NEW
    execfile(game::autoexec(), false);
    loadhistory();                     //NEW
    mod::chat::init();                 //NEW
    initing = NOT_INITING;

    identflags &= ~IDF_PERSIST;

    string gamecfgname;
    copystring(gamecfgname, "data/game_");
    concatstring(gamecfgname, game::gameident());
    concatstring(gamecfgname, ".cfg");
    execfile(gamecfgname);
    
    game::loadconfigs();

    identflags |= IDF_PERSIST;

    if(execfile("once.cfg", false)) remove(findfile("once.cfg", "rb"));

    if(load)
    {
        logoutf("init: localconnect");
        //localconnect();
        game::changemap(load);
    }

    if(initscript) execute(initscript);

    logoutf("init: mainloop");

    //NEW
    // MSVC requires the following to be a function, otherwise the compiler
    // complains about destructible objects used in a __try block
    [](){ conoutf("WC NG SDL2 version: %s", CLIENTVERSION.str().str()); }();
    //NEW END

    initmumble();

    inputgrab(grabinput = true);
    ignoremousemotion();

    execfile(game::wcautoexec(), false); //NEW
    mod::event::run(mod::event::STARTUP); //NEW

    ullong lasticks = mod::getmicroseconds();
    double finelastmillis = lastmillis, finetotalmillis = totalmillis;
    bool drawrequested = false;

    for(;;)
    {
        ullong curticks = mod::getmicroseconds();
        drawrequested |= limitfps(curticks);
        framebenchmark(true); //NEW
        double elapsedmillis = double(curticks - lasticks)/1000.0;
        lasticks = curticks;
        totalmillis = (finetotalmillis += elapsedmillis);
        int oldlastmillis = lastmillis;
        lastmillis = (finelastmillis += game::ispaused() ? 0 : game::scaletime(1) * elapsedmillis / 100);
        bool lightupdate = oldlastmillis == lastmillis;

        updatetime();
 
        checkinput();
        menuprocess();
        tryedit();

        game::updateworld();

        checksleep(lastmillis);

        serverslice(false, 0);

        if(!lightupdate) updatefps(1);

        if(!drawrequested || drawer::swapping())
        {
            if (lightupdate && !microdelay)
            {
#ifdef _MSC_VER
                Sleep(0);
#else
                sched_yield();
#endif
            }
            continue;
        }
        drawrequested = false;

        ullong start = mod::getmicroseconds();

        updatefps(0);

        // miscellaneous general game effects
        recomputecamera();
        updateparticles();
        updatesounds();
    
        mod::slice(); //NEW

        if(minimized && !recorder::isrecording()) //NEW !recorder::isrecording()
        {
            framebenchmark(); //NEW
            continue;
        }

        if(!mainmenu && multipoll > 0) drawer::letdraw();
        else drawer::draw();
        updatefps(2, mod::getmicroseconds() - start);
        framebenchmark(); //NEW
    }
    
    ASSERT(0);   
    return EXIT_FAILURE;

    #if defined(WIN32) && !defined(_DEBUG) && !defined(__GNUC__) && !defined(__clang__) //NEW !defined(__clang__)
    } __except(stackdumper(0, GetExceptionInformation()), EXCEPTION_CONTINUE_SEARCH) { return 0; }
    #endif
}
