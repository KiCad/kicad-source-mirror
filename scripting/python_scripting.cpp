/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 NBEE Embedded Systems, Miguel Angel Ajo <miguelangel@nbee.es>
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

/**
 * @file python_scripting.cpp
 * @brief methods to add scripting capabilities inside Pcbnew
 */

#include <python_scripting.h>

#undef pid_t
#include <pybind11/embed.h>

#include <cstdlib>
#include <cstring>
#include <string>

#include <env_vars.h>
#include <trace_helpers.h>
#include <string_utils.h>
#include <macros.h>

#include <kiface_ids.h>
#include <paths.h>
#include <pgm_base.h>
#include <wx_filename.h>
#include <settings/settings_manager.h>

#include <kiplatform/environment.h>

#include <wx/app.h>
#include <wx/regex.h>
#include <wx/utils.h>

#include <config.h>
#include <gestfich.h>


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
    static bool run = false;
    static bool available = true;

    if( run )
        return available;

    PyLOCK lock;
    using namespace pybind11::literals;

    pybind11::dict locals;

    pybind11::exec( R"(
import traceback
import sys

sys_version = sys.version
wx_version = ""
exception_output = ""

try:
    from wx import version
    wx_version = version()

    # Import wx modules that re-initialize wx globals, because they break wxPropertyGrid
    # (and probably some other stuff) if we let this happen after we already have started
    # mutating those globals.
    import wx.adv, wx.html, wx.richtext

except Exception as e:
    exception_output = "".join(traceback.format_exc())
    )", pybind11::globals(), locals );

    const auto getLocal = [&]( const wxString& aName ) -> wxString
    {
        return wxString( locals[aName.ToStdString().c_str()].cast<std::string>().c_str(),
                         wxConvUTF8 );
    };

    // e.g. "4.0.7 gtk3 (phoenix) wxWidgets 3.0.4"
    wxString version = getLocal( "wx_version" );
    int      idx = version.Find( wxT( "wxWidgets " ) );

    if( idx == wxNOT_FOUND || version.IsEmpty() )
    {
        wxString msg = wxString::Format( wxT( "Could not determine wxWidgets version. "
                                              "Python plugins will not be available." ),
                                         version );

        msg << wxString::Format( wxT( "\n\nsys.version: '%s'" ), getLocal( "sys_version" ) );
        msg << wxString::Format( wxT( "\nwx.version(): '%s'" ), getLocal( "wx_version" ) );

        const wxString exception_output = getLocal( "exception_output" );
        if( !exception_output.IsEmpty() )
            msg << wxT( "\n\n" ) << exception_output;

        wxLogError( msg );
        available = false;
    }
    else
    {
        wxVersionInfo wxVI = wxGetLibraryVersionInfo();
        wxString wxVersion = wxString::Format( wxT( "%d.%d.%d" ),
                                           wxVI.GetMajor(), wxVI.GetMinor(), wxVI.GetMicro() );
        version = version.Mid( idx + 10 );

        long wxPy_major = 0;
        long wxPy_minor = 0;
        long wxPy_micro = 0;
        long wxPy_rev   = 0;

        // Compile a regex to extract the wxPython version
        wxRegEx re( "([0-9]+)\\.([0-9]+)\\.?([0-9]+)?\\.?([0-9]+)?" );
        wxASSERT( re.IsValid() );

        if( re.Matches( version ) )
        {
            wxString v = re.GetMatch( version, 1 );

            if( !v.IsEmpty() )
                v.ToLong( &wxPy_major );

            v = re.GetMatch( version, 2 );

            if( !v.IsEmpty() )
                v.ToLong( &wxPy_minor );

            v = re.GetMatch( version, 3 );

            if( !v.IsEmpty() )
                v.ToLong( &wxPy_micro );

            v = re.GetMatch( version, 4 );

            if( !v.IsEmpty() )
                v.ToLong( &wxPy_rev );
        }

        if( ( wxVI.GetMajor() != wxPy_major ) || ( wxVI.GetMinor() != wxPy_minor ) )
        {
            wxString msg = wxT( "The wxPython library was compiled against wxWidgets %s but KiCad is "
                                "using %s.  Python plugins will not be available." );
            wxLogError( wxString::Format( msg, version, wxVersion ) );
            available = false;
        }
    }

    run = true;

    return available;
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

    // @warning Do we want to use our own ExpandEnvVarSubstitutions() here rather than depend
    //          on wxFileName::Normalize() to expand environment variables.
    pyHome.Normalize( FN_NORMALIZE_FLAGS | wxPATH_NORM_ENV_VARS );

    // MUST be called before Py_Initialize so it will to create valid default lib paths
    if( !wxGetEnv( wxT( "KICAD_USE_EXTERNAL_PYTHONHOME" ), nullptr ) )
    {
        // Global config flag to ignore PYTHONPATH & PYTHONHOME
        Py_IgnoreEnvironmentFlag = 1;

        // Extra insurance to ignore PYTHONPATH and PYTHONHOME
        wxSetEnv( wxT( "PYTHONPATH" ), wxEmptyString );
        wxSetEnv( wxT( "PYTHONHOME" ), wxEmptyString );

        // Now initialize Python Home via capi
        Py_SetPythonHome( pyHome.GetFullPath().c_str() );
    }

    // Allow executing the python pip installed scripts on windows easily
    wxString envPath;
    if( wxGetEnv( wxT( "PATH" ), &envPath ) )
    {
        wxFileName pythonThirdPartyBin( PATHS::GetDefault3rdPartyPath() );
        pythonThirdPartyBin.AppendDir( wxString::Format( wxT( "Python%d%d" ), PY_MAJOR_VERSION, PY_MINOR_VERSION ) );
        pythonThirdPartyBin.AppendDir( wxT( "Scripts" ) );

        envPath = pythonThirdPartyBin.GetAbsolutePath() + ";" + envPath;

        wxSetEnv( wxT( "PATH" ), envPath );
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

    // Prevent Mac builds from generating JIT versions as this will break
    // the package signing
    wxSetEnv( wxT( "PYTHONDONTWRITEBYTECODE" ), wxT( "1" ) );

    // Add default paths to PYTHONPATH
    wxString pypath;

    // Bundle scripting folder (<kicad.app>/Contents/SharedSupport/scripting)
    pypath += PATHS::GetOSXKicadDataDir() + wxT( "/scripting" );

    // $(KICAD_PATH)/scripting/plugins is always added in kicadplugins.i
    if( wxGetenv( "KICAD_PATH" ) != nullptr )
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

    // Hack for run from build dir option
    if( wxGetEnv( wxT( "KICAD_RUN_FROM_BUILD_DIR" ), nullptr ) )
    {
        pypath = wxString( wxT( PYTHON_SITE_PACKAGE_PATH ) ) + wxT( "/../:" )
                 + wxT( PYTHON_SITE_PACKAGE_PATH ) + wxT( ":" ) + wxT( PYTHON_DEST );
    }

    // set $PYTHONPATH
    wxSetEnv( wxT( "PYTHONPATH" ), pypath );

    wxString pyhome;

    pyhome += Pgm().GetExecutablePath() +
              wxT( "Contents/Frameworks/Python.framework/Versions/Current" );

    if( wxGetEnv( wxT( "KICAD_RUN_FROM_BUILD_DIR" ), nullptr ) )
    {
        pyhome = wxString( wxT( PYTHON_SITE_PACKAGE_PATH ) ) + wxT( "/../../../" );
    }

    // set $PYTHONHOME
    wxSetEnv( wxT( "PYTHONHOME" ), pyhome );
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
        ret = From_UTF8( str_res );
        Py_DECREF( temp_bytes );
    }
    else
    {
        wxLogMessage( wxS( "cannot encode Unicode python string" ) );
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
                ret.Add( From_UTF8( str_res ), 1 );
                Py_DECREF( temp_bytes );
            }
            else
            {
                wxLogMessage( wxS( "cannot encode Unicode python string" ) );
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
    case STOCK:
        path = PATHS::GetStockScriptingPath();
        break;

    case USER:
        path = PATHS::GetUserScriptingPath();
        break;

    case THIRDPARTY:
    {
        const ENV_VAR_MAP& env = Pgm().GetLocalEnvVariables();

        if( std::optional<wxString> v = ENV_VAR::GetVersionedEnvVarValue( env, wxT( "3RD_PARTY" ) ) )
            path = *v;
        else
            path = PATHS::GetDefault3rdPartyPath();

        break;
    }
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
