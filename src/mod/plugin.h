/***********************************************************************
 *  WC-NG - Cube 2: Sauerbraten Modification                           *
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

#ifdef _WIN32
#ifdef _WIN64
#define PLUGINEXT ".64bit.dll"
#else
#define PLUGINEXT ".dll"
#endif
#elif defined(__APPLE__)
#define PLUGINEXT ".dylib"
#else
#define PLUGINEXT ".so"
#endif //_WIN32

#define PLUGINDIR "plugins"

#ifdef _WIN32
#define EXPORT extern "C" __declspec (dllexport)
#else
#define EXPORT extern "C"
#endif //_WIN32

namespace mod {
namespace plugin {

struct plugin_t;
struct hostexports_t;

/*
 * The plugin API version.
 * Should be checked in the plugin's init function.
 */
static const int PLUGIN_API_VERSION = 1;

/*
 * Plugin init return codes.
 */
enum init_code_t : int
{
    PLUGIN_INIT_OK,               /* Everything ok */
    PLUGIN_INIT_ERROR,            /* An error occurred during initing */
    PLUGIN_INIT_ERROR_PLUGIN_API, /* Unmatched plugin API version */
};

/*
 * Plugin unload return codes.
 */
enum unload_code_t : int
{
    PLUGIN_UNLOAD_OK,           /* Everything ok */
    PLUGIN_UNLOAD_ERROR,        /* An error occurred during unloading */
    PLUGIN_UNLOAD_ERROR_BUSY    /* Plugin not unloadable */
};

/*
 * Typedefs.
 */
typedef init_code_t (*init_func_t)(const hostexports_t*, plugin_t*);
typedef void (*slice_func_t)();
typedef unload_code_t (*unload_func_t)();
typedef void (*iterate_func_t)(plugin_t*);
typedef clientversion plugin_version_t;
typedef bool (*plugin_cmd_func_t)(const char*, const char*);

/*
 * Plugin struct.
 */
struct plugin_t
{
    strtool name;
    plugin_version_t version;
    clientversion clientver;
    const char *clientrev;
    void *handle;
    unload_func_t unload;               /* Optional */
    slice_func_t slice;                 /* Optional */
    plugin_cmd_func_t command;          /* Optional */
    const char *const *commandnames;    /* Optional */
    uchar reserved[64];                 /* Reserved */

#ifndef PLUGIN
    strtool versionstr();
    void *dlsym(const char *name);
    strtool dlerror();

    plugin_t(const char *name, const char *file);
    ~plugin_t();
#endif //!PLUGIN
};

/*
 * Hostexports struct.
 */
struct hostexports_t
{
    const int API_VERSION = PLUGIN_API_VERSION;

    struct client
    {
        void (*conoutf)(int, const char*, ...) = ::conoutf;
        void (*conoutf_r)(int, const char*, ...) = mod::conoutf_r;
        void (*erroroutf_r)(const char*, ...) = mod::erroroutf_r;
    } client;

    struct event
    {
        declfun(mod::event::validateeventsystem, validateeventsystem);
        int (*run)(uint, const char*, ...) = mod::event::run;
        int (*runstr)(char*, size_t, uint, const char*, ...) = mod::event::run;
    } event;

    struct command
    {
        declfun(getcommandpath, getpath);
        declfun(runcommand, run);
    } command;

    struct http
    {
        declfun(mod::http::request, request);
        declfun(mod::http::uninstallrequest, uninstallrequest);
    } http;

    struct cubescript
    {
        void (*result)(const char*) = ::result;
    } cubescript;

#ifndef PLUGIN
    hostexports_t() {}
#else
    hostexports_t() = delete;
#endif
};

static inline bool checkapiversion(const hostexports_t *he)
{
    return he->API_VERSION == PLUGIN_API_VERSION;
}

/*
 * ...
 */

#ifndef PLUGIN

plugin_t *getplugin(const char *name, bool = false);
void iterateplugins(iterate_func_t *itfun);
int command(const char *pluginname, const char *cmd, const char *val);
bool loadplugin(const char *name);
bool unloadplugin(const char *name, unload_code_t *rc = NULL, bool = false);
int loadplugins();
void unloadplugins(bool shutdown = false);
bool require(const char *pluginname);
void slice();

#endif //PLUGIN

} // plugin
} // mod
