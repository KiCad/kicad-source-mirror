/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 KiCad Developers, see CHANGELOG.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "pcb_scripting_tool.h"

#include <action_plugin.h>
#include <gestfich.h>
#include <kiface_ids.h>
#include <kiway.h>
#include <macros.h>
#include <python_scripting.h>
#include <tools/pcb_actions.h>

#include <pybind11/eval.h>

#include <Python.h>
#include <wx/string.h>

using initfunc = PyObject* (*)(void);

SCRIPTING_TOOL::SCRIPTING_TOOL() :
        PCB_TOOL_BASE( "pcbnew.ScriptingTool" )
{}


SCRIPTING_TOOL::~SCRIPTING_TOOL()
{}


void SCRIPTING_TOOL::Reset( RESET_REASON aReason )
{
}


bool SCRIPTING_TOOL::Init()
{
    PyLOCK    lock;
    std::string pymodule( "_pcbnew" );

    if( !SCRIPTING::IsModuleLoaded( pymodule ) )
    {
        KIFACE* kiface = frame()->Kiway().KiFACE( KIWAY::FACE_PCB );
        initfunc pcbnew_init = reinterpret_cast<initfunc>( kiface->IfaceOrAddress( KIFACE_SCRIPTING_LEGACY ) );
        PyImport_AddModule( pymodule.c_str() );
        PyObject* mod = pcbnew_init();
        PyObject* sys_mod = PyImport_GetModuleDict();
        PyDict_SetItemString( sys_mod, "_pcbnew", mod );
        Py_DECREF( mod );
    }

    // Load pcbnew inside Python and load all the user plugins and package-based plugins
    {
        using namespace pybind11::literals;

        auto locals = pybind11::dict( "sys_path"_a = TO_UTF8( SCRIPTING::PyScriptingPath( false ) ),
                                      "user_path"_a = TO_UTF8( SCRIPTING::PyScriptingPath( true ) ) );

        pybind11::exec( R"(
import sys
import pcbnew
pcbnew.LoadPlugins( sys_path, user_path )
        )", pybind11::globals(), locals );

    }

    return true;
}


int SCRIPTING_TOOL::reloadPlugins( const TOOL_EVENT& aEvent )
{
    if( !m_isFootprintEditor )
        // Reload Python plugins if they are newer than the already loaded, and load new plugins
        // Remove all action plugins so that we don't keep references to old versions
        ACTION_PLUGINS::UnloadAll();

    {
        PyLOCK      lock;
        std::string sys_path = SCRIPTING::PyScriptingPath( false ).ToStdString();
        std::string user_path = SCRIPTING::PyScriptingPath( true ).ToStdString();

        using namespace pybind11::literals;
        auto locals = pybind11::dict( "sys_path"_a = sys_path,
                                      "user_path"_a = user_path );

        pybind11::exec( R"(
import sys
import pcbnew
pcbnew.LoadPlugins( sys_path, user_path )
       )", pybind11::globals(), locals );
    }

    if( !m_isFootprintEditor )
    {
        // Action plugins can be modified, therefore the plugins menu must be updated:
        frame()->ReCreateMenuBar();
        // Recreate top toolbar to add action plugin buttons
        frame()->ReCreateHToolbar();
    }

    return 0;
}


int SCRIPTING_TOOL::showPluginFolder( const TOOL_EVENT& aEvent )
{
#ifdef __WXMAC__
    wxString msg;

    // Quote in case there are spaces in the path.
    msg.Printf( "open \"%s\"", SCRIPTING::PyPluginsPath( true ) );

    system( msg.c_str() );
#else
    wxString pypath( SCRIPTING::PyPluginsPath( true ) );

    // Quote in case there are spaces in the path.
    AddDelimiterString( pypath );

    wxLaunchDefaultApplication( pypath );
#endif

    return 0;
}


void SCRIPTING_TOOL::setTransitions()
{

    Go( &SCRIPTING_TOOL::reloadPlugins,     PCB_ACTIONS::pluginsReload.MakeEvent() );
    Go( &SCRIPTING_TOOL::showPluginFolder,  PCB_ACTIONS::pluginsShowFolder.MakeEvent() );
}
