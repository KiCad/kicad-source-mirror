/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 NBEE Embedded Systems, Miguel Angel Ajo <miguelangel@nbee.es>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <python_scripting.h>
#include <stdlib.h>
#include <string.h>

#include <fctsys.h>
#include <wxstruct.h>
#include <common.h>
#include <colors.h>
#include <macros.h>

/* init functions defined by swig */

extern "C" void init_kicad( void );

extern "C" void init_pcbnew( void );

#define EXTRA_PYTHON_MODULES 10     // this is the number of python
                                    // modules that we want to add into the list


/* python inittab that links module names to module init functions
 * we will rebuild it to include the original python modules plus
 * our own ones
 */

struct _inittab*    SwigImportInittab;
static int          SwigNumModules = 0;

static bool wxPythonLoaded = false;      // true if the wxPython scripting layer was successfully loaded

bool IsWxPythonLoaded()
{
    return wxPythonLoaded;
}


/* Add a name + initfuction to our SwigImportInittab */

static void swigAddModule( const char* name, void (* initfunc)() )
{
    SwigImportInittab[SwigNumModules].name      = (char*) name;
    SwigImportInittab[SwigNumModules].initfunc  = initfunc;
    SwigNumModules++;
    SwigImportInittab[SwigNumModules].name      = (char*) 0;
    SwigImportInittab[SwigNumModules].initfunc  = 0;
}


/* Add the builtin python modules */

static void swigAddBuiltin()
{
    int i = 0;

    /* discover the length of the pyimport inittab */
    while( PyImport_Inittab[i].name )
        i++;

    /* allocate memory for the python module table */
    SwigImportInittab = (struct _inittab*) malloc(
        sizeof(struct _inittab) * (i + EXTRA_PYTHON_MODULES) );

    /* copy all pre-existing python modules into our newly created table */
    i = 0;

    while( PyImport_Inittab[i].name )
    {
        swigAddModule( PyImport_Inittab[i].name, PyImport_Inittab[i].initfunc );
        i++;
    }
}


/* Function swigAddModules
 * adds the internal modules we offer to the python scripting, so they will be
 * available to the scripts we run.
 *
 */

static void swigAddModules()
{
    swigAddModule( "_pcbnew", init_pcbnew );

    // finally it seems better to include all in just one module
    // but in case we needed to include any other modules,
    // it must be done like this:
    // swigAddModule( "_kicad", init_kicad );
}


/* Function swigSwitchPythonBuiltin
 * switches python module table to our built one .
 *
 */

static void swigSwitchPythonBuiltin()
{
    PyImport_Inittab = SwigImportInittab;
}


/* Function pcbnewInitPythonScripting
 * Initializes all the python environment and publish our interface inside it
 * initializes all the wxpython interface, and returns the python thread control structure
 *
 */

PyThreadState* g_PythonMainTState;

bool pcbnewInitPythonScripting( const char * aUserScriptingPath )
{
    swigAddBuiltin();           // add builtin functions
    swigAddModules();           // add our own modules
    swigSwitchPythonBuiltin();  // switch the python builtin modules to our new list

    Py_Initialize();

#ifdef KICAD_SCRIPTING_WXPYTHON
    PyEval_InitThreads();

#ifndef __WINDOWS__     // import wxversion.py currently not working under winbuilder, and not useful.
    char cmd[1024];
    // Make sure that that the correct version of wxPython is loaded. In systems where there
    // are different versions of wxPython installed this can lead to select wrong wxPython
    // version being selected.
    snprintf( cmd, sizeof(cmd), "import wxversion;  wxversion.select('%s')", WXPYTHON_VERSION );

    int retv = PyRun_SimpleString( cmd );

    if( retv != 0 )
    {
        wxLogError( wxT( "Python error %d occurred running string `%s`" ), retv, cmd );
        PyErr_Print();
        Py_Finalize();
        return false;
    }
#endif      // ifndef __WINDOWS__

    // Load the wxPython core API.  Imports the wx._core_ module and sets a
    // local pointer to a function table located there.  The pointer is used
    // internally by the rest of the API functions.
    if( !wxPyCoreAPI_IMPORT() )
    {
        wxLogError( wxT( "***** Error importing the wxPython API! *****" ) );
        PyErr_Print();
        Py_Finalize();
        return false;
    }

    wxPythonLoaded = true;

    // Save the current Python thread state and release the
    // Global Interpreter Lock.

    g_PythonMainTState = wxPyBeginAllowThreads();
#endif  // ifdef KICAD_SCRIPTING_WXPYTHON

    // load pcbnew inside python, and load all the user plugins, TODO: add system wide plugins
    {
        char cmd[1024];
        PyLOCK lock;
        snprintf( cmd, sizeof(cmd), "import sys, traceback\n"
                      "sys.path.append(\".\")\n"
                      "import pcbnew\n"
                      "pcbnew.LoadPlugins(\"%s\")", aUserScriptingPath );
        PyRun_SimpleString( cmd );
    }

    return true;
}


void pcbnewFinishPythonScripting()
{
#ifdef KICAD_SCRIPTING_WXPYTHON
    wxPyEndAllowThreads( g_PythonMainTState );
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

    PyRun_SimpleString( python_redirect );
}


wxWindow* CreatePythonShellWindow( wxWindow* parent, const wxString& aFramenameId )
{
    const char* pcbnew_pyshell =
        "import kicad_pyshell\n"
        "\n"
        "def makeWindow(parent):\n"
        "    return kicad_pyshell.makePcbnewShellWindow(parent)\n"
        "\n";

    wxWindow*   window = NULL;
    PyObject*   result;

    // As always, first grab the GIL
    PyLOCK      lock;

    // Now make a dictionary to serve as the global namespace when the code is
    // executed.  Put a reference to the builtins module in it.

    PyObject*   globals     = PyDict_New();
    PyObject*   builtins    = PyImport_ImportModule( "__builtin__" );

    PyDict_SetItemString( globals, "__builtins__", builtins );
    Py_DECREF( builtins );

    // Execute the code to make the makeWindow function we defined above
    result = PyRun_String( pcbnew_pyshell, Py_file_input, globals, globals );

    // Was there an exception?
    if( !result )
    {
        PyErr_Print();
        return NULL;
    }

    Py_DECREF( result );

    // Now there should be an object named 'makeWindow' in the dictionary that
    // we can grab a pointer to:
    PyObject* func = PyDict_GetItemString( globals, "makeWindow" );
    wxASSERT( PyCallable_Check( func ) );

    // Now build an argument tuple and call the Python function.  Notice the
    // use of another wxPython API to take a wxWindows object and build a
    // wxPython object that wraps it.

    PyObject*   arg = wxPyMake_wxObject( parent, false );
    wxASSERT( arg != NULL );

    PyObject*   tuple = PyTuple_New( 1 );
    PyTuple_SET_ITEM( tuple, 0, arg );

    result = PyEval_CallObject( func, tuple );

    // Was there an exception?
    if( !result )
        PyErr_Print();
    else
    {
        // Otherwise, get the returned window out of Python-land and
        // into C++-ville...
        bool success = wxPyConvertSwigPtr( result, (void**) &window, _T( "wxWindow" ) );
        (void) success;

        wxASSERT_MSG( success, _T( "Returned object was not a wxWindow!" ) );
        Py_DECREF( result );

        window->SetName( aFramenameId );
    }

    // Release the python objects we still have
    Py_DECREF( globals );
    Py_DECREF( tuple );

    return window;
}


#endif

wxArrayString PyArrayStringToWx( PyObject* aArrayString )
{
    wxArrayString   ret;

    int             list_size = PyList_Size( aArrayString );

    for( int n = 0; n<list_size; n++ )
    {
        PyObject* element = PyList_GetItem( aArrayString, n );

        ret.Add( FROM_UTF8( PyString_AsString( element ) ), 1 );
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

    PyObject*   tracebackModuleString = PyString_FromString( (char*) "traceback" );
    PyObject*   tracebackModule = PyImport_Import( tracebackModuleString );


    PyObject*   formatException = PyObject_GetAttrString( tracebackModule,
                                                          (char*) "format_exception" );
    PyObject*   args = Py_BuildValue( "(O,O,O)", type, value, traceback );

    PyObject*   result = PyObject_CallObject( formatException, args );

    Py_DECREF( args );

    wxArrayString res = PyArrayStringToWx( result );

    for( unsigned i = 0; i<res.Count(); i++ )
    {
        err += res[i] + wxT( "\n" );
    }

    PyErr_Clear();

    return err;
}
