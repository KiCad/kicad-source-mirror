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
 * @file pcbnew_scripting.cpp
 * @brief methods to add scripting capabilities inside pcbnew
 */

#include <python_scripting.h>

#include <cstdlib>
#include <cstring>
#include <Python.h>
#include <sstream>

#include <eda_base_frame.h>
#include <gal/color4d.h>
#include <trace_helpers.h>
#include <string_utils.h>
#include <macros.h>
#include <paths.h>
#include <settings/settings_manager.h>

#include <kiplatform/environment.h>

#include <wx/app.h>

#include <config.h>


/**
 * Run a python method from the pcbnew module.
 *
 * @param aMethodName is the name of the method (like "pcbnew.myfunction" ).
 * @param aNames will contain the returned string.
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
        const char* str_res = nullptr;

        if(str)
        {
            PyObject* temp_bytes = PyUnicode_AsEncodedString( str, "UTF-8", "strict" );

            if( temp_bytes != nullptr )
            {
                str_res = PyBytes_AS_STRING( temp_bytes );
                aNames = From_UTF8( str_res );
                Py_DECREF( temp_bytes );
            }
            else
            {
                wxLogMessage( wxS( "cannot encode Unicode python string" ) );
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
    {
        if( strcmp( aMethodName, "pcbnew.GetWizardsBackTrace" ) == 0 )
            aNames = PyErrStringWithTraceback();
        else
            wxLogMessage( PyErrStringWithTraceback() );
    }
}


void pcbnewGetUnloadableScriptNames( wxString& aNames )
{
    pcbnewRunPythonMethodWithReturnedString( "pcbnew.GetUnLoadableWizards", aNames );
}


void pcbnewGetScriptsSearchPaths( wxString& aNames )
{
    pcbnewRunPythonMethodWithReturnedString( "pcbnew.GetWizardsSearchPaths", aNames );
}


void pcbnewGetWizardsBackTrace( wxString& aTrace )
{
    pcbnewRunPythonMethodWithReturnedString( "pcbnew.GetWizardsBackTrace", aTrace );

    // Filter message before displaying them
    // a trace starts by "Traceback" and is followed by 2 useless lines
    // for our purpose
    wxArrayString traces;
    wxStringSplit( aTrace, traces, '\n' );

    // Build the filtered message (remove useless lines)
    aTrace.Clear();

    for( unsigned ii = 0; ii < traces.Count(); ++ii )
    {
        if( traces[ii].Contains( wxT( "Traceback" ) ) )
        {
            ii += 2; // Skip this line and next lines which are related to pcbnew.py module

            if( !aTrace.IsEmpty() ) // Add separator for the next trace block
                aTrace << wxT( "\n**********************************\n" );
        }
        else
        {
            aTrace += traces[ii] + wxT( "\n" );
        }
    }
}
