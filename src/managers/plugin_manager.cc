/*
** Copyright (C) 2014 Cisco and/or its affiliates. All rights reserved.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License Version 2 as
** published by the Free Software Foundation.  You may not use, modify or
** distribute this program under any other version of the GNU General
** Public License.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
*/
// plugin_manager.cc author Russ Combs <rucombs@cisco.com>

#include "plugin_manager.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <sys/stat.h>
#include <dlfcn.h>

#include <map>
#include <vector>
#include <iostream>
using namespace std;

#include "action_manager.h"
#include "data_manager.h"
#include "event_manager.h"
#include "inspector_manager.h"
#include "ips_manager.h"
#include "module_manager.h"
#include "mpse_manager.h"
#include "packet_manager.h"
#include "script_manager.h"
#include "so_manager.h"

#include "framework/codec.h"
#include "framework/logger.h"
#include "framework/ips_action.h"
#include "framework/ips_option.h"
#include "framework/inspector.h"
#include "framework/mpse.h"
#include "framework/plug_data.h"
#include "framework/so_rule.h"

#include "loggers/loggers.h"
#include "ips_options/ips_options.h"
#include "actions/ips_actions.h"
#include "log/messages.h"
#include "stream/stream_inspectors.h"
#include "network_inspectors/network_inspectors.h"
#include "service_inspectors/service_inspectors.h"
#include "search_engines/search_engines.h"
#include "codecs/codec_api.h"
#include "helpers/directory.h"
#include "helpers/markup.h"
#include "parser/parser.h"

#if defined(LINUX)
static const char* lib_ext = ".so";
#else
static const char* lib_ext = ".dylib";
#endif

struct Symbol
{
    const char* name;
    unsigned version;
};

static Symbol symbols[PT_MAX] =
{
    // sequence must match PlugType definition
    { "data", 0 },
    { "codec", CDAPI_VERSION },
    { "inspector", INSAPI_VERSION },
    { "ips_action", ACTAPI_VERSION },
    { "ips_option", IPSAPI_VERSION },
    { "search_engine", SEAPI_VERSION },
    { "so_rule", SOAPI_VERSION },
    { "logger", LOGAPI_VERSION }
};
 
const char* PluginManager::get_type_name(PlugType pt)
{
    if ( pt >= PT_MAX )
        return "error";

    return symbols[pt].name;
}

static const char* current_plugin = nullptr;

const char* PluginManager::get_current_plugin()
{ return current_plugin; }

struct Plugin
{
    string key;
    const BaseApi* api;
    void* handle;

    Plugin()
    { clear(); };

    void clear()
    { key.clear(); api = nullptr; handle = nullptr; };
};

typedef map<string, Plugin> PlugMap;
static PlugMap plug_map;

struct RefCount
{
    unsigned count;

    RefCount() { count = 0; };

    // FIXIT fails on fatal error
    //~RefCount() { assert(!count); };
};

typedef map<void*, RefCount> RefMap;
static RefMap ref_map;

static void set_key(string& key, Symbol* sym, const char* name)
{
    key = sym->name;
    key += "::";
    key += name;
}

static bool register_plugin(
    const BaseApi* api, void* handle = nullptr)
{
    if ( api->type >= PT_MAX )
        return false;

    Symbol* sym = symbols + api->type;

    if ( api->api_version != sym->version )
        return false;

    // validate api ?

    string key;
    set_key(key, sym, api->name);

    Plugin& p = plug_map[key];

    if ( p.api )
    {
        if ( p.api->version >= api->version)
            return false;  // keep the old one

        if ( p.handle && !--ref_map[p.handle].count )
            dlclose(p.handle); // drop the old one
    }

    p.key = key;
    p.api = api;
    p.handle = handle;

    if ( handle )
        ++ref_map[handle].count;

    return true;
}

static void load_list(const BaseApi** api, void* handle = nullptr)
{
    bool keep = false;

    while ( *api )
    {
        keep = register_plugin(*api, handle) || keep;
        ++api;
    }
    if ( handle && !keep )
        dlclose(handle);
}

static bool load_lib(const char* file)
{
    struct stat fs;
    void *handle;

    if ( stat(file, &fs) || !(fs.st_mode & S_IFREG) )
        return false;

    if ( !(handle = dlopen(file, RTLD_NOW|RTLD_LOCAL)) )
    {
        WarningMessage("%s\n", dlerror());
        return false;
    }
    const BaseApi** api = (const BaseApi**)dlsym(handle, "snort_plugins");

    if ( !api )
    {
        dlclose(handle);
        return false;
    }
    load_list(api, handle);
    return true;
}

static void add_plugin(Plugin& p)
{
    if ( p.api->mod_ctor )
    {
        current_plugin = p.api->name;
        Module* m = p.api->mod_ctor();
        ModuleManager::add_module(m, p.api);
    }
    switch ( p.api->type )
    {
    case PT_DATA:
        DataManager::add_plugin((DataApi*)p.api);
        break;

    case PT_CODEC:
        PacketManager::add_plugin((CodecApi*)p.api);
        break;

    case PT_INSPECTOR:
        InspectorManager::add_plugin((InspectApi*)p.api);
        break;

    case PT_IPS_ACTION:
        ActionManager::add_plugin((ActionApi*)p.api);
        break;

    case PT_IPS_OPTION:
        IpsManager::add_plugin((IpsApi*)p.api);
        break;

    case PT_SEARCH_ENGINE:
        MpseManager::add_plugin((MpseApi*)p.api);
        break;

    case PT_SO_RULE:
        SoManager::add_plugin((SoApi*)p.api);
        break;

    case PT_LOGGER:
        EventManager::add_plugin((LogApi*)p.api);
        break;

    default:
        assert(false);
        break;
    }
}

static void load_plugins(const char* s)
{
    if ( !s )
        return;

    vector<char> buf(s, s+strlen(s)+1);
    char* last;
    
    s = strtok_r(&buf[0], ":", &last);

    while ( s )
    {
        Directory d(s);
        const char* f;

        while ( (f = d.next(lib_ext)) )
            load_lib(f);

        s = strtok_r(nullptr, ":", &last);
    }
}

static void add_plugins()
{
    PlugMap::iterator it;

    for ( it = plug_map.begin(); it != plug_map.end(); ++it )
        add_plugin(it->second);
}

static void unload_plugins()
{
    for ( PlugMap::iterator it = plug_map.begin(); it != plug_map.end(); ++it )
    {
        if ( it->second.handle )
            --ref_map[it->second.handle].count;

        it->second.clear();
    }
    
    for ( RefMap::iterator it = ref_map.begin(); it != ref_map.end(); ++it )
    {
        dlclose(it->first);
    }
}

//-------------------------------------------------------------------------
// framework methods
//-------------------------------------------------------------------------

void PluginManager::load_plugins(const char* paths)
{
    // builtins
    load_list(codecs);
    load_list(ips_actions);
    load_list(ips_options);
    load_list(stream_inspectors);
    load_list(network_inspectors);
    load_list(service_inspectors);
    load_list(search_engines);
    load_list(loggers);

    // plugins
    ::load_plugins(paths);

    // scripts
    load_list(ScriptManager::get_plugins());

    add_plugins();
}

// FIXIT some plugins don't have modules; consider adding them
// for stray parameters, perfstats, documentation, etc.
void PluginManager::list_plugins()
{
    PlugMap::iterator it;

    for ( it = plug_map.begin(); it != plug_map.end(); ++it )
    {
        Plugin& p = it->second;
        cout << Markup::item();
        cout << p.key;
        if ( p.api->mod_ctor )
            cout << " (module)";
        cout << endl;
    }
}

void PluginManager::dump_plugins()
{
    DataManager::dump_plugins();
    PacketManager::dump_plugins();
    InspectorManager::dump_plugins();
    MpseManager::dump_plugins();
    IpsManager::dump_plugins();
    SoManager::dump_plugins();
    ActionManager::dump_plugins();
    EventManager::dump_plugins();
}

void PluginManager::release_plugins ()
{
    EventManager::release_plugins();
    ActionManager::release_plugins();
    InspectorManager::release_plugins();
    IpsManager::release_plugins();
    SoManager::release_plugins();
    MpseManager::release_plugins();
    PacketManager::release_plugins();

    // must follow the above
    DataManager::release_plugins();

    unload_plugins();
}

const BaseApi* PluginManager::get_api(PlugType type, const char* name)
{
    if ( type >= PT_MAX )
        return nullptr;

    string key;
    set_key(key, symbols+type, name);

    const PlugMap::iterator it = plug_map.find(key);

    if ( it != plug_map.end() )
        return it->second.api;

    return nullptr;
}

void PluginManager::instantiate(
    const BaseApi* api, Module* mod, SnortConfig* sc)
{
    switch ( api->type )
    {
    case PT_DATA:
        DataManager::instantiate((DataApi*)api, mod, sc);
        break;

    case PT_CODEC:
        PacketManager::instantiate((CodecApi*)api, mod, sc);
        break;

    case PT_INSPECTOR:
        InspectorManager::instantiate((InspectApi*)api, mod, sc);
        break;

    case PT_IPS_ACTION:
        ActionManager::instantiate((ActionApi*)api, mod, sc);
        break;

    case PT_IPS_OPTION:
        // do not instantiate here; done later
        //IpsManager::instantiate((IpsApi*)api, mod, sc);
        break;

    case PT_SEARCH_ENGINE:
        MpseManager::instantiate((MpseApi*)api, mod, sc);
        break;

    case PT_SO_RULE:
        // do not instantiate here; done later
        //IpsManager::instantiate((SoApi*)api, mod, sc);
        break;

    case PT_LOGGER:
        // do not instantiate here; done later
        //EventManager::instantiate((LogApi*)api, mod, sc);
        break;

    default:
        assert(false);
        break;
    }
}

