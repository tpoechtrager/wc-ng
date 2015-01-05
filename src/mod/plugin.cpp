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

#include "cube.h"
#include <dlfcn.h>

namespace mod {
namespace plugin {

static const hostexports_t hostexports;
static vector<plugin_t*> plugins;

plugin_t *getplugin(const char *pluginname, bool msg)
{
    for (auto *p : plugins) if (p->name == pluginname) return p;
    if (msg) erroroutf_r("%s: plugin is not loaded", pluginname);
    return NULL;
}

void iterateplugins(iterate_func_t itfun)
{
    for (auto *p : plugins) itfun(p);
}

int command(const char *pluginname, const char *cmd, const char *val)
{
    plugin_t *plugin = getplugin(pluginname, true);
    if (!plugin) return -1;
    return plugin->command && plugin->command(cmd, val);
}

static inline bool checkpluginname(const char *pluginname)
{
    if (pluginname[0]) return true;
    erroroutf_r("expected plugin name");
    return false;
}

static void updatelistcompletion()
{
    strtool cmd;

    auto listcomplete = [&](const char *command)
    {
        cmd << "listcomplete " << command << ' ';
        for (auto *p : plugins) cmd << p->name << ' ';
        cmd << '"';
        execute(cmd.str());
        cmd.clear();
    };

    listcomplete("plugincommand");
    listcomplete("listplugincommands");
    listcomplete("unloadplugin");
}

bool loadplugin(const char *pluginname)
{
    if (!checkpluginname(pluginname)) return false;

    strtool filename;
    filename << PLUGINDIR <<  '/' << pluginname << PLUGINEXT;

    const char *p = findfile(filename.c_str(), "rb");
    if (p != filename.getrawbuf()) filename = p;

    if (!fileexists(filename.str(), "rb"))
    {
        erroroutf_r("cannot find plugin %s", filename.str());
        return false;
    }

    const char *error = NULL;
    plugin_t *plugin = new plugin_t(pluginname, filename.str());

    if (getplugin(plugin->name.str()))
        error = "plugin is already loaded";

    if (!plugin->handle)
    {
        conoutf(CON_ERROR, "unable to load plugin (%s): %s",
                           pluginname, plugin->dlerror().str());
        delete plugin;
        return false;
    }

    init_func_t init = (init_func_t)plugin->dlsym("init");

    if (!init)
        error = "no init function in plugin";

    if (!error)
    {
        switch (init(&hostexports, plugin))
        {
            case PLUGIN_INIT_OK:
                break;
            case PLUGIN_INIT_ERROR:
                error = "failed to init";
                break;
            case PLUGIN_INIT_ERROR_PLUGIN_API:
                error = "plugin api does not match, recompile";
                break;
            default:
                error = "invalid return code (probably out-of-date)";
        }
    }

    if (error)
    {
        conoutf(CON_ERROR, "unable to load plugin (%s): %s", pluginname, error);
        delete plugin;
        return false;
    }

    plugins.add(plugin);
    updatelistcompletion();

    conoutf("loaded plugin: %s (%s)", pluginname, plugin->versionstr().str());
    event::run(event::PLUGIN_LOAD, "s", pluginname);

    return true;
}

bool unloadplugin(const char *pluginname, unload_code_t *rc, bool shutdown)
{
    if (!checkpluginname(pluginname)) return false;

    plugin_t *plugin = getplugin(pluginname);
    const char *error = NULL;

    if (!plugin)
        error = "plugin not loaded";

    strtool safestr;

    if (plugin)
    {
        safestr = plugin->name;
        pluginname = safestr.str();
    }

    if (!error)
    {
        auto unload = plugin->unload;

        if (unload)
        {
            unload_code_t c = unload();
            if (rc) *rc = c;

            switch (c)
            {
                case PLUGIN_UNLOAD_OK:
                    break;
                case PLUGIN_UNLOAD_ERROR_BUSY:
                    error = "plugin is busy";
                    break;
                default:
                    abort();
            }
        }

        if (!error)
        {
            delete plugin;
            plugins.removeobj(plugin);
            updatelistcompletion();
        }

        if (error && rc) return false;
    }

    if (error)
    {
        conoutf(CON_ERROR, "unable to unload plugin (%s): %s",
                           pluginname, error);
        return false;
    }

    conoutf("unloaded plugin: %s", pluginname);
    event::run(event::PLUGIN_UNLOAD, "sd", pluginname, (int)shutdown);

    return true;
}

int loadplugins()
{
    int loaded = 0;
    vector<char*> files;
    distinctlistfiles(PLUGINDIR, PLUGINEXT+1, files);
    loopv(files)
    {
        const char *plugin = files[i];
#if defined(WIN32) && !defined(_WIN64)
        if (strstr(plugin, "64bit")) continue;
#endif //WIN32
        if(getplugin(plugin)) continue;
        loaded += loadplugin(plugin);
    }
    files.deletearrays();
    return loaded;
}

void unloadplugins(bool shutdown)
{
    loopvrev(plugins)
    {
        auto plugin = plugins[i];
        unload_code_t rc = PLUGIN_UNLOAD_ERROR;

        do
        {
            if (!unloadplugin(plugin->name.str(), &rc, shutdown))
            {
                if (rc != PLUGIN_UNLOAD_ERROR_BUSY)
                    abort();

                conoutf(CON_ERROR, "unable to unload plugin (%s): "
                                   "plugin is busy", plugin->name.str());
                SDL_Delay(1000);
            }
        } while (rc == PLUGIN_UNLOAD_ERROR_BUSY);
    }
}

bool require(const char *pluginname)
{
    if (getplugin(pluginname)) return true;
    return loadplugin(pluginname);
}

void slice()
{
    iterateplugins([](plugin_t *p) { if (p->slice) p->slice(); });
}

//

plugin_t::plugin_t(const char *name, const char *file) :
    name(name), handle(dlopen(file, RTLD_LAZY)),
    unload(NULL), slice(), command(), commandnames() {}

plugin_t::~plugin_t()
{
    if (handle) dlclose(handle);
}

strtool plugin_t::versionstr()
{
    strtool s;
    s << version.str() << " -- " << clientver.str();
    return s;
}

void *plugin_t::dlsym(const char *name)
{
    return ::dlsym(handle, name);
}

strtool plugin_t::dlerror()
{
    strtool s;
    s << "dlerror(): " << ::dlerror();
    return s;
}

//

static void plugincommand(const char *pname, const char *cmd, const char *val)
{
    if (!checkpluginname(pname)) return;

    switch (command(pname, cmd, val))
    {
        case 0: erroroutf_r("%s: plugin is not loaded", pname); break;
        default:;
    }
}
COMMAND(plugincommand, "sss");

static void listloadedplugins(int *val)
{
    if (*val == 1)
    {
        strtool pluginnames;
        for (auto *p : plugins) pluginnames << p->name << ' ';
        if (pluginnames) pluginnames.pop();
        result(pluginnames.str());
        return;
    }

    if (plugins.empty())
    {
        conoutf("no plugins loaded");
        return;
    }
    for (auto *p : plugins)
    {
        conoutf("%s (version: %s)", p->name.str(), p->versionstr().str());
    }
}
COMMAND(listloadedplugins, "i");

static void listplugincommands(const char *pluginname, int *val)
{
    if (!checkpluginname(pluginname)) return;

    plugin_t *plugin = getplugin(pluginname, true);
    if (!plugin) return;

    strtool commandnames;

    if (plugin->commandnames) for (size_t i = 0; plugin->commandnames[i]; i++)
    {
        commandnames += plugin->commandnames[i];
        if (*val != 1) commandnames += ',';
        commandnames += ' ';
    }

    if (commandnames) commandnames -= (*val == 1 ? 1 : 2);

    if (*val == 1)
    {
        result(commandnames.str());
        return;
    }

    if (!commandnames) commandnames = "this plugin does not support commands";
    conoutf("%s: supported commands: %s", pluginname, commandnames.str());
}
COMMAND(listplugincommands, "si");

static void loadplugin_(const char *name)
{
    intret(loadplugin(name));
}
COMMANDN(loadplugin, loadplugin_, "s");

static void unloadplugin_(const char *name)
{
    intret(unloadplugin(name));
}
COMMANDN(unloadplugin, unloadplugin_, "s");

ICOMMAND(loadplugins, "", (), intret(loadplugins()));
ICOMMAND(unloadplugins, "", (), unloadplugins(false));
ICOMMAND(requireplugin, "s", (const char *n), intret(require(n)));
ICOMMAND(ispluginloaded, "s", (const char *n), intret(!!getplugin(n)));

/* Deprecated */
COMMANDN(listplugins, listloadedplugins, "i");
COMMANDN(addplugin, loadplugin_, "s");
COMMANDN(delplugin, unloadplugin_, "s");
ICOMMAND(addplugins, "", (), intret(loadplugins()));
ICOMMAND(delplugins, "", (), unloadplugins(false));
ICOMMAND(isplugin, "s", (const char *n), intret(!!getplugin(n)));

} // plugin
} // mod
