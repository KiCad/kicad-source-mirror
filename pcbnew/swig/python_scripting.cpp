/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 NBEE Embedded Systems, Miguel Angel Ajo <miguelangel@nbee.es>
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @brief methods to add scripting capabilities inside pcbnew
 */

#include <cstdlib>
#include <cstring>
#include <python_scripting.h>
#include <sstream>

#include <fctsys.h>
#include <eda_base_frame.h>
#include <common.h>
#include <gal/color4d.h>
#include <macros.h>

#include <pgm_base.h>

/* init functions defined by swig */

#if PY_MAJOR_VERSION >= 3
extern "C" PyObject* PyInit__pcbnew( void );
#else
extern "C" void init_kicad( void );

extern "C" void init_pcbnew( void );
#endif

#define EXTRA_PYTHON_MODULES 10     // this is the number of python
                                    // modules that we want to add into the list


/* python inittab that links module names to module init functions
 * we will rebuild it to include the original python modules plus
 * our own ones
 */

struct _inittab*    SwigImportInittab;
static int          SwigNumModules = 0;

/// True if the wxPython scripting layer was successfully loaded.
static bool wxPythonLoaded = false;


bool IsWxPythonLoaded()
{
    return wxPythonLoaded;
}


/**
 * Add a name + initfuction to our SwigImportInittab
 */

#if PY_MAJOR_VERSION >= 3
static void swigAddModule( const char* name, PyObject* (* initfunc)() )
#else
static void swigAddModule( const char* name, void (* initfunc)() )
#endif
{
    SwigImportInittab[SwigNumModules].name      = (char*) name;
    SwigImportInittab[SwigNumModules].initfunc  = initfunc;
    SwigNumModules++;
    SwigImportInittab[SwigNumModules].name      = (char*) 0;
    SwigImportInittab[SwigNumModules].initfunc  = 0;
}


/**
 * Add the builtin python modules
 */
static void swigAddBuiltin()
{
    int i = 0;

    /* discover the length of the pyimport inittab */
    while( PyImport_Inittab[i].name )
        i++;

    /* allocate memory for the python module table */
    SwigImportInittab = (struct _inittab*) malloc(
        sizeof( struct _inittab ) * ( i + EXTRA_PYTHON_MODULES ) );

    /* copy all pre-existing python modules into our newly created table */
    i = 0;

    while( PyImport_Inittab[i].name )
    {
        swigAddModule( PyImport_Inittab[i].name, PyImport_Inittab[i].initfunc );
        i++;
    }
}


/**
 * Add the internal modules to the python scripting so they will be available to the scripts.
 */
static void swigAddModules()
{
#if PY_MAJOR_VERSION >= 3
    swigAddModule( "_pcbnew", PyInit__pcbnew );
#else
    swigAddModule( "_pcbnew", init_pcbnew );
#endif

    // finally it seems better to include all in just one module
    // but in case we needed to include any other modules,
    // it must be done like this:
    // swigAddModule( "_kicad", init_kicad );
}


/**
 * Switch the python module table to the Pcbnew built one.
 */
static void swigSwitchPythonBuiltin()
{
    PyImport_Inittab = SwigImportInittab;
}


PyThreadState* g_PythonMainTState;


/**
 * Initialize the python environment and publish the Pcbnew interface inside it.
 *
 * This initializes all the wxPython interface and returns the python thread control structure
 */
bool pcbnewInitPythonScripting( const char * aUserScriptingPath )
{
    int  retv;
    char cmd[1024];

    swigAddBuiltin();           // add builtin functions
    swigAddModules();           // add our own modules
    swigSwitchPythonBuiltin();  // switch the python builtin modules to our new list

    Py_Initialize();
    PySys_SetArgv( Pgm().App().argc, Pgm().App().argv );

#ifdef KICAD_SCRIPTING_WXPYTHON
    PyEval_InitThreads();

#ifndef KICAD_SCRIPTING_WXPYTHON_PHOENIX
#ifndef __WINDOWS__   // import wxversion.py currently not working under winbuilder, and not useful.
    // Make sure that that the correct version of wxPython is loaded. In systems where there
    // are different versions of wxPython installed this can lead to select wrong wxPython
    // version being selected.
    snprintf( cmd, sizeof( cmd ), "import wxversion;  wxversion.select( '%s' )", WXPYTHON_VERSION );

    retv = PyRun_SimpleString( cmd );

    if( retv != 0 )
    {
        wxLogError( "Python error %d occurred running command:\n\n`%s`", retv, cmd );
        return false;
    }
#endif      // ifndef __WINDOWS__

    // Load the wxPython core API.  Imports the wx._core_ module and sets a
    // local pointer to a function table located there.  The pointer is used
    // internally by the rest of the API functions.
    if( !wxPyCoreAPI_IMPORT() )
    {
        wxLogError( "***** Error importing the wxPython API! *****" );
        PyErr_Print();
        Py_Finalize();
        return false;
    }
#endif

#if defined( DEBUG )
    RedirectStdio();
#endif

    wxPythonLoaded = true;

    // Save the current Python thread state and release the
    // Global Interpreter Lock.
    g_PythonMainTState = PyEval_SaveThread();

#endif  // ifdef KICAD_SCRIPTING_WXPYTHON

    // Load pcbnew inside Python and load all the user plugins, TODO: add system wide plugins
    {
        PyLOCK lock;

        snprintf( cmd, sizeof( cmd ), "import sys, traceback\n"
                  "sys.path.append(\".\")\n"
                  "import pcbnew\n"
                  "pcbnew.LoadPlugins(\"%s\")", aUserScriptingPath );
        retv = PyRun_SimpleString( cmd );

        if( retv != 0 )
            wxLogError( "Python error %d occurred running command:\n\n`%s`", retv, cmd );
    }

    return true;
}


/**
 * Run a python method from the pcbnew module.
 *
 * @param aMethodName is the name of the method (like "pcbnew.myfunction" )
 * @param aNames will contain the returned string
 */
static void pcbnewRunPythonMethodWithReturnedString( const char* aMethodName, wxString& aNames )
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
#if PY_MAJOR_VERSION >= 3
        const char* str_res = NULL;

        if(str)
        {
            PyObject* temp_bytes = PyUnicode_AsEncodedString( str, "UTF-8", "strict" );

            if( temp_bytes != NULL )
            {
                str_res = PyBytes_AS_STRING( temp_bytes );
                str_res = strdup( str_res );
                Py_DECREF( temp_bytes );
            }
            else
            {
                wxLogMessage( "cannot encode unicode python string" );
            }
        }
#else
        const char* str_res = str ? PyString_AsString( str ) : 0;
#endif
        aNames = FROM_UTF8( str_res );
        Py_DECREF( pobj );
    }

    Py_DECREF( localDict );

    if( PyErr_Occurred() )
        wxLogMessage( PyErrStringWithTraceback() );
}


void pcbnewGetUnloadableScriptNames( wxString& aNames )
{
    pcbnewRunPythonMethodWithReturnedString( "pcbnew.GetUnLoadableWizards", aNames );
}


void pcbnewGetScriptsSearchPaths( wxString& aNames )
{
    pcbnewRunPythonMethodWithReturnedString( "pcbnew.GetWizardsSearchPaths", aNames );
}


void pcbnewGetWizardsBackTrace( wxString& aNames )
{
    pcbnewRunPythonMethodWithReturnedString( "pcbnew.GetWizardsBackTrace", aNames );
}


void pcbnewFinishPythonScripting()
{
#ifdef KICAD_SCRIPTING_WXPYTHON
    PyEval_RestoreThread( g_PythonMainTState );
#endif
    Py_Finalize();
}


#if defined( KICAD_SCRIPTING_WXPYTHON )
void RedirectStdio()
{
    // This is a helpful little tidbit to help debugging and such.  It
    // redirects Python's stdout and stderr to a window that will popup
    // only on demand when something is printed, like a traceback.
    const char* python_redirect =
        "import sys\n"
        "import wx\n"
        "output = wx.PyOnDemandOutputWindow()\n"
        "sys.stderr = output\n";

    PyLOCK lock;

    int retv = PyRun_SimpleString( python_redirect );

    if( retv != 0 )
        wxLogError( "Python error %d occurred running command:\n\n`%s`", retv, python_redirect );
}


wxWindow* CreatePythonShellWindow( wxWindow* parent, const wxString& aFramenameId )
{
    // parent is actually *PCB_EDIT_FRAME
    const int parentId = parent->GetId();
    {
        wxWindow* parent2 = wxWindow::FindWindowById( parentId );
        wxASSERT( parent2 == parent );
    }

    // passing window ids instead of pointers is because wxPython is not
    // exposing the needed c++ apis to make that possible.
    std::stringstream pcbnew_pyshell_one_step;
    pcbnew_pyshell_one_step << "import kicad_pyshell\n";
    pcbnew_pyshell_one_step << "import wx\n";
    pcbnew_pyshell_one_step << "\n";
    pcbnew_pyshell_one_step << "parent = wx.FindWindowById( " << parentId << " )\n";
    pcbnew_pyshell_one_step << "newshell = kicad_pyshell.makePcbnewShellWindow( parent )\n";
    pcbnew_pyshell_one_step << "newshell.SetName( \"" << aFramenameId << "\" )\n";
    // return value goes into a "global". It's not actually global, but rather
    // the dict that is passed to PyRun_String
    pcbnew_pyshell_one_step << "retval = newshell.GetId()\n";

    // As always, first grab the GIL
    PyLOCK      lock;

    // Now make a dictionary to serve as the global namespace when the code is
    // executed.  Put a reference to the builtins module in it.

    PyObject*   globals     = PyDict_New();
#if PY_MAJOR_VERSION >= 3
    PyObject*   builtins    = PyImport_ImportModule( "builtins" );
#else
    PyObject*   builtins    = PyImport_ImportModule( "__builtin__" );
#endif

    wxASSERT( builtins );

    PyDict_SetItemString( globals, "__builtins__", builtins );
    Py_DECREF( builtins );

#ifdef KICAD_SCRIPTING_WXPYTHON_PHOENIX
    auto app_ptr = wxTheApp;
#endif
    // Execute the code to make the makeWindow function we defined above
    PyObject*   result = PyRun_String( pcbnew_pyshell_one_step.str().c_str(), Py_file_input,
                                       globals, globals );

#ifdef KICAD_SCRIPTING_WXPYTHON_PHOENIX
    // This absolutely ugly hack is to work around the pyshell re-writing the global
    // wxApp variable.  Once we can initialize Phoenix to access the appropriate instance
    // of wx, we can remove this line
    wxApp::SetInstance( app_ptr );
#endif
    // Was there an exception?
    if( !result )
    {
        PyErr_Print();
        return NULL;
    }

    Py_DECREF( result );

    result = PyDict_GetItemString( globals, "retval" );

#if PY_MAJOR_VERSION >= 3
    if( !PyLong_Check( result ) )
#else
    if( !PyInt_Check( result ) )
#endif
    {
        wxLogError( "creation of scripting window didn't return a number" );
        return NULL;
    }

#if PY_MAJOR_VERSION >= 3
    const long windowId = PyLong_AsLong( result );
#else
    const long windowId = PyInt_AsLong( result );
#endif

    // It's important not to decref globals before extracting the window id.
    // If you do it early, globals, and the retval int it contains, may/will be garbage collected.
    // We do not need to decref result, because GetItemString returns a borrowed reference.
    Py_DECREF( globals );

    wxWindow* window = wxWindow::FindWindowById( windowId );

    if( !window )
    {
        wxLogError( "unable to find pyshell window with id %d", windowId );
        return NULL;
    }

    return window;
}
#endif


wxString PyStringToWx( PyObject* aString )
{
    wxString    ret;

    if( !aString )
        return ret;

#if PY_MAJOR_VERSION >= 3
    const char* str_res = NULL;
    PyObject* temp_bytes = PyUnicode_AsEncodedString( aString, "UTF-8", "strict" );

    if( temp_bytes != NULL )
    {
        str_res = PyBytes_AS_STRING( temp_bytes );
        ret = FROM_UTF8( str_res );
        Py_DECREF( temp_bytes );
    }
    else
    {
        wxLogMessage( "cannot encode unicode python string" );
    }
#else
    const char* str_res = PyString_AsString( aString );
    ret = FROM_UTF8( str_res );
#endif

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
#if PY_MAJOR_VERSION >= 3
            const char* str_res = NULL;
            PyObject* temp_bytes = PyUnicode_AsEncodedString( element, "UTF-8", "strict" );

            if( temp_bytes != NULL )
            {
                str_res = PyBytes_AS_STRING( temp_bytes );
                ret.Add( FROM_UTF8( str_res ), 1 );
                Py_DECREF( temp_bytes );
            }
            else
            {
                wxLogMessage( "cannot encode unicode python string" );
            }
#else
            ret.Add( FROM_UTF8( PyString_AsString( element ) ), 1 );
#endif
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

    if( traceback == NULL )
    {
        traceback = Py_None;
        Py_INCREF( traceback );
    }

#if PY_MAJOR_VERSION >= 3
    PyException_SetTraceback( value, traceback );

    PyObject* tracebackModuleString = PyUnicode_FromString( "traceback" );
#else
    PyObject* tracebackModuleString = PyString_FromString( "traceback" );
#endif
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
wxString PyScriptingPath()
{
    wxString path;

    //@todo This should this be a user configurable variable eg KISCRIPT?
#if defined( __WXMAC__ )
    path = GetOSXKicadDataDir() + wxT( "/scripting" );
#else
    path = Pgm().GetExecutablePath() + wxT( "../share/kicad/scripting" );
#endif

    wxFileName scriptPath( path );
    scriptPath.MakeAbsolute();

    // Convert '\' to '/' in path, because later python script read \n or \r
    // as escaped sequence, and create issues, when calling it by PyRun_SimpleString() method.
    // It can happen on Windows.
    path = scriptPath.GetFullPath();
    path.Replace( '\\', '/' );

    return path;
}


wxString PyPluginsPath()
{
    // Note we are using unix path separator, because window separator sometimes
    // creates issues when passing a command string to a python method by PyRun_SimpleString
    return PyScriptingPath() + '/' + "plugins";
}
