/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 NBEE Embedded Systems, Miguel Angel Ajo <miguelangel@nbee.es>
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <wx/wxPython/wxPython.h>
#include <stdlib.h>
#include <string.h>

/* init functions defined by swig */

extern "C" void init_kicad( void );

extern "C" void init_pcbnew( void );

#define EXTRA_PYTHON_MODULES 10  // this is the number of python
                                // modules that we want to add into the list


/* python inittab that links module names to module init functions
 * we will rebuild it to include the original python modules plus
 * our own ones
 */

struct _inittab *SwigImportInittab;
static int      SwigNumModules = 0;


/* Add a name + initfuction to our SwigImportInittab */

static void swigAddModule( const char* name, void (* initfunc)() )
{
    SwigImportInittab[SwigNumModules].name     = (char*) name;
    SwigImportInittab[SwigNumModules].initfunc = initfunc;
    SwigNumModules++;
    SwigImportInittab[SwigNumModules].name     = (char*) 0;
    SwigImportInittab[SwigNumModules].initfunc = 0;
}


/* Add the builting python modules */

static void swigAddBuiltin()
{
    int i = 0;

    /* discover the length of the pyimport inittab */
    while( PyImport_Inittab[i].name )	i++;

    /* allocate memory for the python module table */
    SwigImportInittab = (struct _inittab*)  malloc(
                        sizeof(struct _inittab)*(i+EXTRA_PYTHON_MODULES));
    
    /* copy all pre-existing python modules into our newly created table */
    i=0;
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
    // swigAddModule("_kicad",init_kicad);
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

PyThreadState *g_PythonMainTState;

bool pcbnewInitPythonScripting()
{
 
    swigAddBuiltin();           // add builtin functions
    swigAddModules();           // add our own modules
    swigSwitchPythonBuiltin();  // switch the python builtin modules to our new list

    Py_Initialize();
    PyEval_InitThreads();

    // Load the wxPython core API.  Imports the wx._core_ module and sets a
    // local pointer to a function table located there.  The pointer is used
    // internally by the rest of the API functions.
    if ( ! wxPyCoreAPI_IMPORT() ) {
        wxLogError(wxT("***** Error importing the wxPython API! *****"));
        PyErr_Print();
        Py_Finalize();
        return false;
    }        
    
    // Save the current Python thread state and release the
    // Global Interpreter Lock.

    g_PythonMainTState = wxPyBeginAllowThreads();

    // load pcbnew inside python, and load all the user plugins, TODO: add system wide plugins

    wxPyBlock_t blocked = wxPyBeginBlockThreads();
    PyRun_SimpleString( "import sys\n"
                        "sys.path.append(\".\")\n"
                        "import pcbnew\n"
                        "pcbnew.LoadPlugins()"
                        );
    wxPyEndBlockThreads(blocked);

    return true;
}

void pcbnewFinishPythonScripting()
{

    wxPyEndAllowThreads(g_PythonMainTState);
    Py_Finalize();

}
