/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 NBEE Embedded Systems SL, Miguel Angel Ajo <miguelangel@ajo.es>
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
 * @file  pcbnew_footprint_wizards.cpp
 * @brief Class  and PYTHON_FOOTPRINT_WIZARD_LIST and PYTHON_FOOTPRINT_WIZARD_LIST
 */

#include "pcbnew_footprint_wizards.h"
#include <cstdio>
#include <macros.h>
#include <wx/msgdlg.h>
#include "../../scripting/python_scripting.h"


PYTHON_FOOTPRINT_WIZARD::PYTHON_FOOTPRINT_WIZARD( PyObject* aWizard )
{
    PyLOCK lock;

    m_PyWizard = aWizard;
    Py_XINCREF( aWizard );
}


PYTHON_FOOTPRINT_WIZARD::~PYTHON_FOOTPRINT_WIZARD()
{
    PyLOCK lock;

    Py_XDECREF( m_PyWizard );
}


PyObject* PYTHON_FOOTPRINT_WIZARD::CallMethod( const char* aMethod, PyObject* aArglist )
{
    PyLOCK lock;

    PyErr_Clear();
    // pFunc is a new reference to the desired method
    PyObject* pFunc = PyObject_GetAttrString( m_PyWizard, aMethod );

    if( pFunc && PyCallable_Check( pFunc ) )
    {
        PyObject* result = PyObject_CallObject( pFunc, aArglist );

        if( PyErr_Occurred() )
        {
#if 1 // defined(DEBUG)
            wxMessageBox( PyErrStringWithTraceback(),
                          _( "Exception on python footprint wizard code" ),
                          wxICON_ERROR | wxOK );
#endif
        }

        if( result )
        {
            Py_XDECREF( pFunc );
            return result;
        }
    }
    else
    {
        wxString msg = wxString::Format(_( "Method '%s' not found, or not callable" ), aMethod );
        wxMessageBox( msg, _( "Unknown Method" ), wxICON_ERROR | wxOK );
    }

    if( pFunc )
    {
        Py_XDECREF( pFunc );
    }

    return nullptr;
}


wxString PYTHON_FOOTPRINT_WIZARD::CallRetStrMethod( const char* aMethod, PyObject* aArglist )
{
    wxString    ret;
    PyLOCK      lock;

    PyObject*   result = CallMethod( aMethod, aArglist );

    if( result == Py_None )
    {
        Py_DECREF( result );
        return ret;
    }

    ret = PyStringToWx( result );
    Py_XDECREF( result );

    return ret;
}


wxArrayString PYTHON_FOOTPRINT_WIZARD::CallRetArrayStrMethod( const char*   aMethod,
                                                              PyObject*     aArglist )
{
    wxArrayString   ret;
    PyLOCK          lock;

    PyObject*       result = CallMethod( aMethod, aArglist );

    if( result )
    {
        if( !PyList_Check( result ) )
        {
            Py_DECREF( result );
            ret.Add( wxT( "PYTHON_FOOTPRINT_WIZARD::CallRetArrayStrMethod, result is not a list" ),
                     1 );
            return ret;
        }

        ret = PyArrayStringToWx( result );

        Py_DECREF( result );
    }

    return ret;
}


wxString PYTHON_FOOTPRINT_WIZARD::GetName()
{
    PyLOCK lock;

    return CallRetStrMethod( "GetName" );
}


wxString PYTHON_FOOTPRINT_WIZARD::GetImage()
{
    PyLOCK lock;

    return CallRetStrMethod( "GetImage" );
}


wxString PYTHON_FOOTPRINT_WIZARD::GetDescription()
{
    PyLOCK lock;

    return CallRetStrMethod( "GetDescription" );
}


int PYTHON_FOOTPRINT_WIZARD::GetNumParameterPages()
{
    int         ret = 0;
    PyLOCK      lock;

    // Time to call the callback
    PyObject*   result = CallMethod( "GetNumParameterPages", nullptr );

    if( result )
    {
        if( !PyLong_Check( result ) )
            return -1;

        ret = PyLong_AsLong( result );
        Py_DECREF( result );
    }

    return ret;
}


wxString PYTHON_FOOTPRINT_WIZARD::GetParameterPageName( int aPage )
{
    wxString    ret;
    PyLOCK      lock;

    // Time to call the callback
    PyObject*   arglist = Py_BuildValue( "(i)", aPage );
    PyObject*   result  = CallMethod( "GetParameterPageName", arglist );

    Py_DECREF( arglist );

    if( result == Py_None )
    {
        Py_DECREF( result );
        return ret;
    }

    ret = PyStringToWx( result );
    Py_XDECREF( result );

    return ret;
}


wxArrayString PYTHON_FOOTPRINT_WIZARD::GetParameterNames( int aPage )
{
    wxArrayString   ret;
    PyLOCK          lock;

    PyObject*       arglist = Py_BuildValue( "(i)", aPage );

    ret = CallRetArrayStrMethod( "GetParameterNames", arglist );
    Py_DECREF( arglist );

    for( unsigned i = 0; i < ret.GetCount(); i++ )
    {
        wxString    rest;
        wxString    item = ret[i];

        if( item.StartsWith( wxT( "*" ), &rest ) )
        {
            ret[i] = rest;
        }
    }

    return ret;
}


wxArrayString PYTHON_FOOTPRINT_WIZARD::GetParameterTypes( int aPage )
{
    wxArrayString   ret;
    PyLOCK          lock;

    PyObject*       arglist = Py_BuildValue( "(i)", aPage );

    ret = CallRetArrayStrMethod( "GetParameterTypes", arglist );
    Py_DECREF( arglist );

    return ret;
}


wxArrayString PYTHON_FOOTPRINT_WIZARD::GetParameterValues( int aPage )
{
    PyLOCK          lock;

    PyObject*       arglist = Py_BuildValue( "(i)", aPage );
    wxArrayString   ret = CallRetArrayStrMethod( "GetParameterValues", arglist );

    Py_DECREF( arglist );

    return ret;
}


wxArrayString PYTHON_FOOTPRINT_WIZARD::GetParameterErrors( int aPage )
{
    PyLOCK          lock;

    PyObject*       arglist = Py_BuildValue( "(i)", aPage );
    wxArrayString   ret = CallRetArrayStrMethod( "GetParameterErrors", arglist );

    Py_DECREF( arglist );

    return ret;
}

wxArrayString PYTHON_FOOTPRINT_WIZARD::GetParameterHints( int aPage )
{
    PyLOCK          lock;

    PyObject*       arglist = Py_BuildValue( "(i)", aPage );
    wxArrayString   ret = CallRetArrayStrMethod( "GetParameterHints", arglist );

    Py_DECREF( arglist );

    return ret;
}

wxArrayString PYTHON_FOOTPRINT_WIZARD::GetParameterDesignators( int aPage )
{
    PyLOCK          lock;

    PyObject*       arglist = Py_BuildValue( "(i)", aPage );
    wxArrayString   ret = CallRetArrayStrMethod( "GetParameterDesignators", arglist );

    Py_DECREF( arglist );

    return ret;
}

wxString PYTHON_FOOTPRINT_WIZARD::SetParameterValues( int aPage, wxArrayString& aValues )
{
    int         len = aValues.size();

    PyLOCK      lock;

    PyObject*   py_list = PyList_New( len );

    for( int i = 0; i < len; i++ )
    {
        wxString&    str     = aValues[i];
        PyObject*   py_str  = PyUnicode_FromString( (const char*) str.mb_str() );
        PyList_SetItem( py_list, i, py_str );
    }

    PyObject*   arglist;

    arglist = Py_BuildValue( "(i,O)", aPage, py_list );
    wxString    res = CallRetStrMethod( "SetParameterValues", arglist );
    Py_DECREF( arglist );

    return res;
}

void PYTHON_FOOTPRINT_WIZARD::ResetParameters()
{
    PyLOCK     lock;

    CallMethod( "ResetWizard", nullptr );
}


// this is a SWIG function declaration -from footprint.i
FOOTPRINT* PyFootprint_to_FOOTPRINT( PyObject* obj0 );


FOOTPRINT* PYTHON_FOOTPRINT_WIZARD::GetFootprint( wxString * aMessages )
{
    PyLOCK      lock;

    PyObject*   result = CallMethod( "GetFootprint", nullptr );

    if( aMessages )
        *aMessages = CallRetStrMethod( "GetBuildMessages", nullptr );

    if( !result )
        return nullptr;

    PyObject* obj = PyObject_GetAttrString( result, "this" );

    if( PyErr_Occurred() )
    {
        PyErr_Print();
        PyErr_Clear();
    }

    FOOTPRINT* footprint = PyFootprint_to_FOOTPRINT( obj );

    return footprint;
}


void* PYTHON_FOOTPRINT_WIZARD::GetObject()
{
    return (void*) m_PyWizard;
}


void PYTHON_FOOTPRINT_WIZARD_LIST::register_wizard( PyObject* aPyWizard )
{
    PYTHON_FOOTPRINT_WIZARD* fw = new PYTHON_FOOTPRINT_WIZARD( aPyWizard );

    fw->register_wizard();
}


void PYTHON_FOOTPRINT_WIZARD_LIST::deregister_wizard( PyObject* aPyWizard )
{
    // deregister also destroys the previously created "PYTHON_FOOTPRINT_WIZARD object"
    FOOTPRINT_WIZARD_LIST::deregister_object( (void*) aPyWizard );
}
