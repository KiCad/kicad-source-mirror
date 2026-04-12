/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "eeschema_action_plugins.h"
#include <python_scripting.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <settings/common_settings.h>
#include <bitmaps.h>
#include <tool/action_menu.h>
#include <tool/action_toolbar.h>
#include <tool/tool_manager.h>
#include <sch_edit_frame.h>
#include <wx/msgdlg.h>
#include <wx/app.h>
#include <kiplatform/app.h>


// ---- PYTHON_SCH_ACTION_PLUGIN implementation ----

PYTHON_SCH_ACTION_PLUGIN::PYTHON_SCH_ACTION_PLUGIN( PyObject* aAction )
{
    PyLOCK lock;
    m_PyAction = aAction;
    Py_XINCREF( aAction );
}


PYTHON_SCH_ACTION_PLUGIN::~PYTHON_SCH_ACTION_PLUGIN()
{
    PyLOCK lock;
    Py_XDECREF( m_PyAction );
}


PyObject* PYTHON_SCH_ACTION_PLUGIN::CallMethod( const char* aMethod, PyObject* aArglist )
{
    PyLOCK lock;

    PyErr_Clear();

    PyObject* pFunc = PyObject_GetAttrString( m_PyAction, aMethod );

    if( pFunc && PyCallable_Check( pFunc ) )
    {
        PyObject* result = PyObject_CallObject( pFunc, aArglist );

        if( !wxTheApp )
            KIPLATFORM::APP::AttachConsole( true );

        if( PyErr_Occurred() )
        {
            wxString message = _HKI( "Exception on eeschema action plugin code" );
            wxString traceback = PyErrStringWithTraceback();

            std::cerr << message << std::endl << std::endl;
            std::cerr << traceback << std::endl;

            if( wxTheApp )
                wxSafeShowMessage( wxGetTranslation( message ), traceback );
        }

        if( !wxTheApp )
        {
            std::cerr << "Fatal: wxApp destroyed after running eeschema plugin '"
                      << m_cachedName << "'" << std::endl;
            abort();
        }

        if( result )
        {
            Py_XDECREF( pFunc );
            return result;
        }
    }
    else
    {
        wxString msg = wxString::Format( _( "Method '%s' not found, or not callable" ), aMethod );
        wxMessageBox( msg, _( "Unknown Method" ), wxICON_ERROR | wxOK );
    }

    if( pFunc )
        Py_XDECREF( pFunc );

    return nullptr;
}


wxString PYTHON_SCH_ACTION_PLUGIN::CallRetStrMethod( const char* aMethod, PyObject* aArglist )
{
    wxString ret;
    PyLOCK   lock;

    PyObject* result = CallMethod( aMethod, aArglist );
    ret = PyStringToWx( result );
    Py_XDECREF( result );

    return ret;
}


wxString PYTHON_SCH_ACTION_PLUGIN::GetCategoryName()
{
    PyLOCK lock;
    return CallRetStrMethod( "GetCategoryName" );
}

wxString PYTHON_SCH_ACTION_PLUGIN::GetClassName()
{
    PyLOCK lock;
    return CallRetStrMethod( "GetClassName" );
}

wxString PYTHON_SCH_ACTION_PLUGIN::GetName()
{
    PyLOCK lock;
    wxString name = CallRetStrMethod( "GetName" );
    m_cachedName = name;
    return name;
}

wxString PYTHON_SCH_ACTION_PLUGIN::GetDescription()
{
    PyLOCK lock;
    return CallRetStrMethod( "GetDescription" );
}

bool PYTHON_SCH_ACTION_PLUGIN::GetShowToolbarButton()
{
    PyLOCK lock;
    PyObject* result = CallMethod( "GetShowToolbarButton" );
    return PyObject_IsTrue( result );
}

wxString PYTHON_SCH_ACTION_PLUGIN::GetIconFileName( bool aDark )
{
    PyLOCK lock;
    PyObject* arglist = Py_BuildValue( "(i)", static_cast<int>( aDark ) );
    wxString  result = CallRetStrMethod( "GetIconFileName", arglist );
    Py_DECREF( arglist );
    return result;
}

wxString PYTHON_SCH_ACTION_PLUGIN::GetPluginPath()
{
    PyLOCK lock;
    return CallRetStrMethod( "GetPluginPath" );
}

void PYTHON_SCH_ACTION_PLUGIN::Run()
{
    PyLOCK lock;
    CallMethod( "Run" );
}

void* PYTHON_SCH_ACTION_PLUGIN::GetObject()
{
    return (void*) m_PyAction;
}


// ---- PYTHON_SCH_ACTION_PLUGINS static methods ----

void PYTHON_SCH_ACTION_PLUGINS::register_action( PyObject* aPyAction )
{
    PYTHON_SCH_ACTION_PLUGIN* plugin = new PYTHON_SCH_ACTION_PLUGIN( aPyAction );
    plugin->register_action();
}


void PYTHON_SCH_ACTION_PLUGINS::deregister_action( PyObject* aPyAction )
{
    SCH_ACTION_PLUGINS::deregister_object( (void*) aPyAction );
}


// ---- Python C extension module: _eeschema_action_plugins ----

static PyObject* py_register_action( PyObject* self, PyObject* args )
{
    PyObject* pyAction;

    if( !PyArg_ParseTuple( args, "O", &pyAction ) )
        return nullptr;

    PYTHON_SCH_ACTION_PLUGINS::register_action( pyAction );
    Py_RETURN_NONE;
}


static PyObject* py_deregister_action( PyObject* self, PyObject* args )
{
    PyObject* pyAction;

    if( !PyArg_ParseTuple( args, "O", &pyAction ) )
        return nullptr;

    PYTHON_SCH_ACTION_PLUGINS::deregister_action( pyAction );
    Py_RETURN_NONE;
}


static PyMethodDef EeschemaActionMethods[] = {
    { "register_action",   py_register_action,   METH_VARARGS,
      "Register an action plugin with eeschema" },
    { "deregister_action", py_deregister_action, METH_VARARGS,
      "Deregister an action plugin from eeschema" },
    { nullptr, nullptr, 0, nullptr }
};


static struct PyModuleDef eeschemaActionModule = {
    PyModuleDef_HEAD_INIT,
    "_eeschema_action_plugins",
    "Eeschema action plugin registration",
    -1,
    EeschemaActionMethods,
    nullptr,   // m_slots
    nullptr,   // m_traverse
    nullptr,   // m_clear
    nullptr    // m_free
};


static PyObject* PyInit_eeschema_action_plugins()
{
    return PyModule_Create( &eeschemaActionModule );
}


// ---- Plugin loading ----

void InitEeschemaActionPlugins()
{
    PyLOCK lock;

    // Register our C extension module into sys.modules so Python can import it.
    // We can't use PyImport_AppendInittab because Python is already initialized.
    PyObject* module = PyInit_eeschema_action_plugins();

    if( module )
    {
        PyObject* sysModules = PyImport_GetModuleDict();
        PyDict_SetItemString( sysModules, "_eeschema_action_plugins", module );
        Py_DECREF( module );
    }
}


void LoadEeschemaActionPlugins()
{
    PyLOCK lock;

    wxString stockPath  = SCRIPTING::PyScriptingPath( SCRIPTING::PATH_TYPE::STOCK );
    wxString userPath   = SCRIPTING::PyScriptingPath( SCRIPTING::PATH_TYPE::USER );
    wxString thirdParty = SCRIPTING::PyPluginsPath( SCRIPTING::PATH_TYPE::THIRDPARTY );

    // Python code that:
    // 1. Imports our C registration module
    // 2. Defines an ActionPlugin base class for eeschema
    // 3. Scans plugin directories and loads plugins that register with eeschema
    wxString pyCode = wxString::Format( R"(
import sys
import os
import traceback
import importlib

import _eeschema_action_plugins

class EeschemaActionPlugin:
    """Base class for eeschema action plugins (mirrors pcbnew.ActionPlugin)."""
    def __init__(self):
        self.name = "Undefined Action plugin"
        self.category = "Undefined"
        self.description = ""
        self.icon_file_name = ""
        self.dark_icon_file_name = ""
        self.show_toolbar_button = False
        self.__plugin_path = ""
        self.defaults()

    def defaults(self):
        pass

    def GetClassName(self):
        return type(self).__name__

    def GetName(self):
        return self.name

    def GetCategoryName(self):
        return self.category

    def GetDescription(self):
        return self.description

    def GetShowToolbarButton(self):
        return self.show_toolbar_button

    def GetIconFileName(self, dark):
        if dark and self.dark_icon_file_name:
            return self.dark_icon_file_name
        return self.icon_file_name

    def GetPluginPath(self):
        return self.__plugin_path

    def Run(self):
        pass

    def register(self):
        import inspect
        self.__plugin_path = inspect.getfile(self.__class__)
        if self.__plugin_path.endswith('.pyc') and os.path.isfile(self.__plugin_path[:-1]):
            self.__plugin_path = self.__plugin_path[:-1]
        self.__plugin_path = self.__plugin_path + '/' + self.__class__.__name__
        _eeschema_action_plugins.register_action(self)

    def deregister(self):
        _eeschema_action_plugins.deregister_action(self)

# Make it available as a module-level name so plugins can subclass it
sys.modules['eeschema_plugins'] = type(sys)('eeschema_plugins')
sys.modules['eeschema_plugins'].ActionPlugin = EeschemaActionPlugin

# Also inject into builtins so existing pcbnew-style plugins can be adapted
# by just checking which environment they're in

# Now scan plugin directories for plugins
plugin_directories = []

stock_path = r'%s'
user_path = r'%s'
third_party_path = r'%s'

config_path = ''
try:
    # Try to get the settings path
    import pcbnew
    config_path = pcbnew.SETTINGS_MANAGER.GetUserSettingsPath()
except:
    pass

if stock_path:
    plugin_directories.append(stock_path)
    plugin_directories.append(os.path.join(stock_path, 'plugins'))

if config_path:
    plugin_directories.append(os.path.join(config_path, 'scripting'))
    plugin_directories.append(os.path.join(config_path, 'scripting', 'plugins'))

if user_path:
    plugin_directories.append(user_path)
    plugin_directories.append(os.path.join(user_path, 'plugins'))

if third_party_path:
    plugin_directories.append(third_party_path)

for plugins_dir in plugin_directories:
    if not os.path.isdir(plugins_dir):
        continue

    if plugins_dir not in sys.path:
        sys.path.append(plugins_dir)

    for module_file in os.listdir(plugins_dir):
        full_path = os.path.join(plugins_dir, module_file)

        if os.path.isdir(full_path):
            if os.path.exists(os.path.join(full_path, '__init__.py')):
                try:
                    importlib.import_module(module_file)
                except:
                    traceback.print_exc()
            continue

        if module_file == '__init__.py' or not module_file.endswith('.py'):
            continue

        mod_name = module_file[:-3]
        try:
            if mod_name in sys.modules:
                importlib.reload(sys.modules[mod_name])
            else:
                importlib.import_module(mod_name)
        except:
            traceback.print_exc()
)",
        stockPath, userPath, thirdParty );

    PyRun_SimpleString( pyCode.utf8_str() );
}


// ---- SCH_EDIT_FRAME action plugin integration ----

void SCH_EDIT_FRAME::OnActionPluginMenu( wxCommandEvent& aEvent )
{
    SCH_ACTION_PLUGIN* actionPlugin = SCH_ACTION_PLUGINS::GetActionByMenu( aEvent.GetId() );

    if( actionPlugin )
        RunActionPlugin( actionPlugin );
}


void SCH_EDIT_FRAME::OnActionPluginButton( wxCommandEvent& aEvent )
{
    SCH_ACTION_PLUGIN* actionPlugin = SCH_ACTION_PLUGINS::GetActionByButton( aEvent.GetId() );

    if( actionPlugin )
        RunActionPlugin( actionPlugin );
}


void SCH_EDIT_FRAME::RunActionPlugin( SCH_ACTION_PLUGIN* aActionPlugin )
{
    // Eeschema plugins are simpler than pcbnew - no board undo/redo tracking needed.
    // The plugin typically just downloads files or shows dialogs.
    SCH_ACTION_PLUGINS::SetActionRunning( true );
    aActionPlugin->Run();
    SCH_ACTION_PLUGINS::SetActionRunning( false );
}


void SCH_EDIT_FRAME::buildActionPluginMenus( ACTION_MENU* actionMenu )
{
    if( !actionMenu )
        return;

    int iconSize = Pgm().GetCommonSettings()->m_Appearance.toolbar_icon_size;

    for( int ii = 0; ii < SCH_ACTION_PLUGINS::GetActionsCount(); ii++ )
    {
        wxMenuItem*       item;
        SCH_ACTION_PLUGIN* ap = SCH_ACTION_PLUGINS::GetAction( ii );

        wxBitmapBundle bundle;

        if( ap->iconBitmap.IsOk() && ap->iconBitmap.GetHeight() > 0 )
        {
            wxBitmap bmp = ap->iconBitmap;
            wxSize   size = bmp.GetSize();
            double   defScale = (double) iconSize / size.GetHeight();
            wxSize   defSize( iconSize, wxRound( size.GetWidth() * defScale ) );

            wxBitmap bmp1 = bmp;
            wxBitmapHelpers::Rescale( bmp1, defSize );

            wxBitmap bmp2 = bmp;
            wxBitmapHelpers::Rescale( bmp2, defSize * 2 );

            bundle = wxBitmapBundle::FromBitmaps( bmp1, bmp2 );
        }
        else
        {
            bundle = KiBitmapBundleDef( BITMAPS::puzzle_piece, iconSize );
        }

        item = KIUI::AddMenuItem( actionMenu, wxID_ANY, ap->GetName(),
                                  ap->GetDescription(), bundle );

        Connect( item->GetId(), wxEVT_COMMAND_MENU_SELECTED,
                 wxCommandEventHandler( SCH_EDIT_FRAME::OnActionPluginMenu ) );

        SCH_ACTION_PLUGINS::SetActionMenu( ii, item->GetId() );
    }
}


void SCH_EDIT_FRAME::addActionPluginTools( ACTION_TOOLBAR* aToolbar )
{
    bool need_separator = true;
    int  iconSize = Pgm().GetCommonSettings()->m_Appearance.toolbar_icon_size;

    for( int ii = 0; ii < SCH_ACTION_PLUGINS::GetActionsCount(); ii++ )
    {
        SCH_ACTION_PLUGIN* ap = SCH_ACTION_PLUGINS::GetAction( ii );

        if( ap->GetShowToolbarButton() )
        {
            if( need_separator )
            {
                aToolbar->AddScaledSeparator( this );
                need_separator = false;
            }

            wxBitmapBundle bundle;

            if( ap->iconBitmap.IsOk() && ap->iconBitmap.GetHeight() > 0 )
            {
                wxBitmap bmp = ap->iconBitmap;
                wxSize   size = bmp.GetSize();
                double   defScale = (double) iconSize / size.GetHeight();
                wxSize   defSize( iconSize, wxRound( size.GetWidth() * defScale ) );

                wxBitmap bmp1 = bmp;
                wxBitmapHelpers::Rescale( bmp1, defSize );

                wxBitmap bmp2 = bmp;
                wxBitmapHelpers::Rescale( bmp2, defSize * 2 );

                bundle = wxBitmapBundle::FromBitmaps( bmp1, bmp2 );
            }
            else
            {
                bundle = KiBitmapBundleDef( BITMAPS::puzzle_piece, iconSize );
            }

            wxAuiToolBarItem* button = aToolbar->AddTool(
                    wxID_ANY, wxEmptyString, bundle, ap->GetName() );

            Connect( button->GetId(), wxEVT_COMMAND_MENU_SELECTED,
                     wxCommandEventHandler( SCH_EDIT_FRAME::OnActionPluginButton ) );

            SCH_ACTION_PLUGINS::SetActionButton( ap, button->GetId() );
        }
    }
}
