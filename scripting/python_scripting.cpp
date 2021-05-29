/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 NBEE Embedded Systems, Miguel Angel Ajo <miguelangel@nbee.es>
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file python_scripting.cpp
 * @brief methods to add scripting capabilities inside Pcbnew
 */

#include <python_scripting.h>

#include <cstdlib>
#include <cstring>
#include <Python.h>
#include <sstream>

#include <eda_base_frame.h>
#include <gal/color4d.h>
#include <gestfich.h>
#include <trace_helpers.h>
#include <string_utils.h>
#include <macros.h>

#include <kiface_ids.h>
#include <paths.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>

#include <kiplatform/environment.h>

#include <wx/app.h>

#include <config.h>


SCRIPTING::SCRIPTING()
{
    scriptingSetup();

    pybind11::initialize_interpreter();

    // Save the current Python thread state and release the Global Interpreter Lock.
    m_python_thread_state = PyEval_SaveThread();
}


SCRIPTING::~SCRIPTING()
{
    PyEval_RestoreThread( m_python_thread_state );

    try
    {
        pybind11::finalize_interpreter();
    }
    catch( const std::runtime_error& exc )
    {
        wxLogError( wxT( "Run time error '%s' occurred closing Python scripting" ), exc.what() );
    }
}


bool SCRIPTING::IsWxAvailable()
{
#ifdef KICAD_SCRIPTING_WXPYTHON
    return true;
#else
    return false;
#endif
}


bool SCRIPTING::IsModuleLoaded( std::string& aModule )
{
    PyLOCK    lock;
    using namespace pybind11::literals;
    auto locals = pybind11::dict( "modulename"_a = aModule );

    pybind11::exec( R"(
import sys
loaded = False
if modulename in sys.modules:
    loaded = True

    )", pybind11::globals(), locals );

    return locals["loaded"].cast<bool>();
}


bool SCRIPTING::scriptingSetup()
{
#if defined( __WINDOWS__ )

  #ifdef _MSC_VER
    // Under vcpkg/msvc, we need to explicitly set the python home or else it'll start consuming
    // system python registry keys and the like instead of the Python distributed with KiCad.
    // We are going to follow the "unix" layout for the msvc/vcpkg distributions so executable
    // files are in the /root/bin path and the Python library files are in the
    // /root/lib/python3(/Lib,/DLLs) path(s).
    wxFileName pyHome;

    pyHome.Assign( Pgm().GetExecutablePath() );

    pyHome.Normalize();

    // MUST be called before Py_Initialize so it will to create valid default lib paths
    if( !wxGetEnv( wxT( "KICAD_USE_EXTERNAL_PYTHONHOME" ), nullptr ) )
    {
        Py_SetPythonHome( pyHome.GetFullPath().c_str() );
    }
  #else
    // Intended for msys2 but we could probably use the msvc equivalent code too
    // If our python.exe (in kicad/bin) exists, force our kicad python environment
    wxString kipython = FindKicadFile( "python.exe" );

    // we need only the path:
    wxFileName fn( kipython  );
    kipython = fn.GetPath();

    // If our python install is existing inside kicad, use it
    // Note: this is useful only when another python version is installed
    if( wxDirExists( kipython ) )
    {
        // clear any PYTHONPATH and PYTHONHOME env var definition: the default
        // values work fine inside Kicad:
        wxSetEnv( wxT( "PYTHONPATH" ), wxEmptyString );
        wxSetEnv( wxT( "PYTHONHOME" ), wxEmptyString );

        // Add our python executable path in first position:
        wxString ppath;
        wxGetEnv( wxT( "PATH" ), &ppath );

        kipython << wxT( ";" ) << ppath;
        wxSetEnv( wxT( "PATH" ), kipython );
    }
  #endif
#elif defined( __WXMAC__ )

    // Add default paths to PYTHONPATH
    wxString pypath;

    // Bundle scripting folder (<kicad.app>/Contents/SharedSupport/scripting)
    pypath += PATHS::GetOSXKicadDataDir() + wxT( "/scripting" );

    // $(KICAD_PATH)/scripting/plugins is always added in kicadplugins.i
    if( wxGetenv("KICAD_PATH") != nullptr )
    {
        pypath += wxT( ":" ) + wxString( wxGetenv("KICAD_PATH") );
    }

    // OSX_BUNDLE_PYTHON_SITE_PACKAGES_DIR is provided via the build system.

    pypath += wxT( ":" ) + Pgm().GetExecutablePath() + wxT( OSX_BUNDLE_PYTHON_SITE_PACKAGES_DIR );

    // Original content of $PYTHONPATH
    if( wxGetenv( wxT( "PYTHONPATH" ) ) != nullptr )
    {
        pypath = wxString( wxGetenv( wxT( "PYTHONPATH" ) ) ) + wxT( ":" ) + pypath;
    }

    // set $PYTHONPATH
    wxSetEnv( "PYTHONPATH", pypath );

    wxString pyhome;

    pyhome += Pgm().GetExecutablePath() +
              wxT( "Contents/Frameworks/Python.framework/Versions/Current" );

    // set $PYTHONHOME
    wxSetEnv( "PYTHONHOME", pyhome );
#else
    wxString pypath;

    if( wxGetEnv( wxT( "KICAD_RUN_FROM_BUILD_DIR" ), nullptr ) )
    {
        // When running from build dir, python module gets built next to Pcbnew binary
        pypath = Pgm().GetExecutablePath() + wxT( "../pcbnew" );
    }
    else
    {
        // PYTHON_DEST is the scripts install dir as determined by the build system.
        pypath = Pgm().GetExecutablePath() + wxT( "../" PYTHON_DEST );
    }

    if( !wxIsEmpty( wxGetenv( wxT( "PYTHONPATH" ) ) ) )
        pypath = wxString( wxGetenv( wxT( "PYTHONPATH" ) ) ) + wxT( ":" ) + pypath;

    wxSetEnv( wxT( "PYTHONPATH" ), pypath );

#endif

    wxFileName path( PyPluginsPath( SCRIPTING::PATH_TYPE::USER ) + wxT( "/" ) );

    // Ensure the user plugin path exists, and create it if not.
    // However, if it cannot be created, this is not a fatal error.
    if( !path.DirExists() && !path.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) )
        wxLogError( _( "Could not create user scripting path %s." ), path.GetPath() );

    return true;
}


/**
 * Run a python method from the Pcbnew module.
 *
 * @param aMethodName is the name of the method (like "pcbnew.myfunction" )
 * @param aNames will contain the returned string
 */
static void RunPythonMethodWithReturnedString( const char* aMethodName, wxString& aNames )
{
    aNames.Clear();

    PyLOCK      lock;
    PyErr_Clear();

    PyObject* builtins = PyImport_ImportModule( "pcbnew" );
    wxASSERT( builtins );

    if( !builtins ) // Something is wrong in pcbnew.py module (incorrect version?)
        return;

    PyObject* globals = PyDict_New();
    PyDict_SetItemString( globals, "pcbnew", builtins );
    Py_DECREF( builtins );

    // Build the python code
    char cmd[1024];
    snprintf( cmd, sizeof(cmd), "result = %s()", aMethodName );

    // Execute the python code and get the returned data
    PyObject* localDict = PyDict_New();
    PyObject* pobj = PyRun_String( cmd,  Py_file_input, globals, localDict);
    Py_DECREF( globals );

    if( pobj )
    {
        PyObject* str = PyDict_GetItemString(localDict, "result" );
        const char* str_res = nullptr;

        if(str)
        {
            PyObject* temp_bytes = PyUnicode_AsEncodedString( str, "UTF-8", "strict" );

            if( temp_bytes != nullptr )
            {
                str_res = PyBytes_AS_STRING( temp_bytes );
                aNames = FROM_UTF8( str_res );
                Py_DECREF( temp_bytes );
            }
            else
            {
                wxLogMessage( "cannot encode Unicode python string" );
            }
        }
        else
        {
            aNames = wxString();
        }

        Py_DECREF( pobj );
    }

    Py_DECREF( localDict );

    if( PyErr_Occurred() )
        wxLogMessage( PyErrStringWithTraceback() );
}


wxString PyEscapeString( const wxString& aSource )
{
    wxString converted;

    for( wxUniChar c: aSource )
    {
        if( c == '\\' )
            converted += "\\\\";
        else if( c == '\'' )
            converted += "\\\'";
        else if( c == '\"' )
            converted += "\\\"";
        else
            converted += c;
    }

    return converted;
}


void UpdatePythonEnvVar( const wxString& aVar, const wxString& aValue )
{
    char cmd[1024];

    // Ensure the interpreter is initialized before we try to interact with it.
    if( !Py_IsInitialized() )
        return;

    wxLogTrace( traceEnvVars, "UpdatePythonEnvVar: Updating Python variable %s = %s",
                aVar, aValue );

    wxString escapedVar = PyEscapeString( aVar );
    wxString escapedVal = PyEscapeString( aValue );

    snprintf( cmd, sizeof( cmd ),
              "# coding=utf-8\n"      // The values could potentially be UTF8.
              "import os\n"
              "os.environ[\"%s\"]=\"%s\"\n",
              TO_UTF8( escapedVar ),
              TO_UTF8( escapedVal ) );

    PyLOCK lock;

    int retv = PyRun_SimpleString( cmd );

    if( retv != 0 )
        wxLogError( "Python error %d running command:\n\n`%s`", retv, cmd );
}


wxString PyStringToWx( PyObject* aString )
{
    wxString    ret;

    if( !aString )
        return ret;

    const char* str_res = nullptr;
    PyObject* temp_bytes = PyUnicode_AsEncodedString( aString, "UTF-8", "strict" );

    if( temp_bytes != nullptr )
    {
        str_res = PyBytes_AS_STRING( temp_bytes );
        ret = FROM_UTF8( str_res );
        Py_DECREF( temp_bytes );
    }
    else
    {
        wxLogMessage( "cannot encode Unicode python string" );
    }

    return ret;
}


wxArrayString PyArrayStringToWx( PyObject* aArrayString )
{
    wxArrayString   ret;

    if( !aArrayString )
        return ret;

    int list_size = PyList_Size( aArrayString );

    for( int n = 0; n < list_size; n++ )
    {
        PyObject* element = PyList_GetItem( aArrayString, n );

        if( element )
        {
            const char* str_res = nullptr;
            PyObject* temp_bytes = PyUnicode_AsEncodedString( element, "UTF-8", "strict" );

            if( temp_bytes != nullptr )
            {
                str_res = PyBytes_AS_STRING( temp_bytes );
                ret.Add( FROM_UTF8( str_res ), 1 );
                Py_DECREF( temp_bytes );
            }
            else
            {
                wxLogMessage( "cannot encode Unicode python string" );
            }
        }
    }

    return ret;
}


wxString PyErrStringWithTraceback()
{
    wxString err;

    if( !PyErr_Occurred() )
        return err;

    PyObject*   type;
    PyObject*   value;
    PyObject*   traceback;

    PyErr_Fetch( &type, &value, &traceback );

    PyErr_NormalizeException( &type, &value, &traceback );

    if( traceback == nullptr )
    {
        traceback = Py_None;
        Py_INCREF( traceback );
    }

    PyException_SetTraceback( value, traceback );

    PyObject* tracebackModuleString = PyUnicode_FromString( "traceback" );
    PyObject* tracebackModule = PyImport_Import( tracebackModuleString );
    Py_DECREF( tracebackModuleString );

    PyObject* formatException = PyObject_GetAttrString( tracebackModule,
                                                        "format_exception" );
    Py_DECREF( tracebackModule );

    PyObject* args = Py_BuildValue( "(O,O,O)", type, value, traceback );
    PyObject* result = PyObject_CallObject( formatException, args );
    Py_XDECREF( formatException );
    Py_XDECREF( args );
    Py_XDECREF( type );
    Py_XDECREF( value );
    Py_XDECREF( traceback );

    wxArrayString res = PyArrayStringToWx( result );

    for( unsigned i = 0; i<res.Count(); i++ )
    {
        err += res[i] + wxT( "\n" );
    }

    PyErr_Clear();

    return err;
}


/**
 * Find the Python scripting path.
 */
wxString SCRIPTING::PyScriptingPath( PATH_TYPE aPathType )
{
    wxString path;

    //@todo This should this be a user configurable variable eg KISCRIPT?
    switch( aPathType )
    {
    case STOCK: path = PATHS::GetStockScriptingPath(); break;
    case USER: path = PATHS::GetUserScriptingPath(); break;
    case THIRDPARTY:
        const ENV_VAR_MAP& env = Pgm().GetLocalEnvVariables();
        auto               it = env.find( "KICAD6_3RD_PARTY" );

        if( it != env.end() && !it->second.GetValue().IsEmpty() )
            path = it->second.GetValue();
        else
            path = PATHS::GetDefault3rdPartyPath();

        break;
    }

    wxFileName scriptPath( path );
    scriptPath.MakeAbsolute();

    // Convert '\' to '/' in path, because later python script read \n or \r
    // as escaped sequence, and create issues, when calling it by PyRun_SimpleString() method.
    // It can happen on Windows.
    path = scriptPath.GetFullPath();
    path.Replace( '\\', '/' );

    return path;
}


wxString SCRIPTING::PyPluginsPath( PATH_TYPE aPathType )
{
    // Note we are using unix path separator, because window separator sometimes
    // creates issues when passing a command string to a python method by PyRun_SimpleString
    return PyScriptingPath( aPathType ) + '/' + "plugins";
}
