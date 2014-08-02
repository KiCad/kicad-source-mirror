/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 NBEE Embedded Systems SL, Miguel Angel Ajo <miguelangel@ajo.es>
 * Copyright (C) 2013 KiCad Developers, see CHANGELOG.TXT for contributors.
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
 * @brief Class PCBNEW_PYTHON_FOOTPRINT_WIZARDS
 */

#include "pcbnew_footprint_wizards.h"
#include <python_scripting.h>
#include <stdio.h>
#include <macros.h>


PYTHON_FOOTPRINT_WIZARD::PYTHON_FOOTPRINT_WIZARD( PyObject* aWizard )
{
    PyLOCK lock;

    this->m_PyWizard = aWizard;
    Py_XINCREF( aWizard );
}


PYTHON_FOOTPRINT_WIZARD::~PYTHON_FOOTPRINT_WIZARD()
{
    PyLOCK lock;

    Py_XDECREF( this->m_PyWizard );
}


PyObject* PYTHON_FOOTPRINT_WIZARD::CallMethod( const char* aMethod, PyObject* aArglist )
{
    PyLOCK lock;

    PyErr_Clear();
    // pFunc is a new reference to the desired method
    PyObject* pFunc = PyObject_GetAttrString( this->m_PyWizard, aMethod );

    if( pFunc && PyCallable_Check( pFunc ) )
    {
        PyObject* result = PyObject_CallObject( pFunc, aArglist );

        if( PyErr_Occurred() )
        {
            wxMessageBox( PyErrStringWithTraceback(),
                          wxT( "Exception on python footprint wizard code" ),
                          wxICON_ERROR | wxOK );
        }

        if( result )
        {
            Py_XDECREF( pFunc );
            return result;
        }
    }
    else
    {
        printf( "method not found, or not callable: %s\n", aMethod );
    }

    if( pFunc )
    {
        Py_XDECREF( pFunc );
    }

    return NULL;
}


wxString PYTHON_FOOTPRINT_WIZARD::CallRetStrMethod( const char* aMethod, PyObject* aArglist )
{
    wxString    ret;
    PyLOCK      lock;

    PyObject*   result = CallMethod( aMethod, aArglist );

    if( result )
    {
        const char* str_res = PyString_AsString( result );
        ret = FROM_UTF8( str_res );
        Py_DECREF( result );
    }

    return ret;
}


wxArrayString PYTHON_FOOTPRINT_WIZARD::CallRetArrayStrMethod( const char*   aMethod,
                                                              PyObject*     aArglist )
{
    wxArrayString   ret;
    wxString        str_item;
    PyLOCK          lock;

    PyObject*       result = CallMethod( aMethod, aArglist );

    if( result )
    {
        if( !PyList_Check( result ) )
        {
            Py_DECREF( result );
            ret.Add( wxT(
                         "PYTHON_FOOTPRINT_WIZARD::CallRetArrayStrMethod, result is not a list" ),
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
    PyObject*   result = CallMethod( "GetNumParameterPages", NULL );

    if( result )
    {
        if( !PyInt_Check( result ) )
            return -1;

        ret = PyInt_AsLong( result );
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

    if( result )
    {
        const char* str_res = PyString_AsString( result );
        ret = FROM_UTF8( str_res );
        Py_DECREF( result );
    }

    return ret;
}


wxArrayString PYTHON_FOOTPRINT_WIZARD::GetParameterNames( int aPage )
{
    wxArrayString   ret;
    PyLOCK          lock;

    PyObject*       arglist = Py_BuildValue( "(i)", aPage );

    ret = CallRetArrayStrMethod( "GetParameterNames", arglist );
    Py_DECREF( arglist );

    for( unsigned i = 0; i<ret.GetCount(); i++ )
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

    ret = CallRetArrayStrMethod( "GetParameterNames", arglist );
    Py_DECREF( arglist );

    for( unsigned i = 0; i<ret.GetCount(); i++ )
    {
        wxString    rest;
        wxString    item = ret[i];

        if( item.StartsWith( wxT( "*" ), &rest ) )
        {
            ret[i] = wxT( "UNITS" );    // units
        }
        else
        {
            ret[i] = wxT( "IU" );       // internal units
        }
    }

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


wxString PYTHON_FOOTPRINT_WIZARD::SetParameterValues( int aPage, wxArrayString& aValues )
{
    int         len = aValues.size();

    PyLOCK      lock;

    PyObject*   py_list = PyList_New( len );

    for( int i = 0; i<len; i++ )
    {
        wxString    str     = aValues[i];
        PyObject*   py_str  = PyString_FromString( (const char*) str.mb_str() );
        PyList_SetItem( py_list, i, py_str );
    }

    PyObject*   arglist;

    arglist = Py_BuildValue( "(i,O)", aPage, py_list );
    wxString    res = CallRetStrMethod( "SetParameterValues", arglist );
    Py_DECREF( arglist );

    return res;
}


// this is a SWIG function declaration -from module.i
MODULE* PyModule_to_MODULE( PyObject* obj0 );


MODULE* PYTHON_FOOTPRINT_WIZARD::GetModule()
{
    PyLOCK      lock;

    PyObject*   result = CallMethod( "GetModule", NULL );

    if( !result )
        return NULL;

    PyObject* obj = PyObject_GetAttrString( result, "this" );

    if( PyErr_Occurred() )
    {
        PyErr_Print();
        PyErr_Clear();
    }

    MODULE* mod = PyModule_to_MODULE( obj );

    return mod;
}


void* PYTHON_FOOTPRINT_WIZARD::GetObject()
{
    return (void*) m_PyWizard;
}


void PYTHON_FOOTPRINT_WIZARDS::register_wizard( PyObject* aPyWizard )
{
    PYTHON_FOOTPRINT_WIZARD* fw = new PYTHON_FOOTPRINT_WIZARD( aPyWizard );

    fw->register_wizard();
}


void PYTHON_FOOTPRINT_WIZARDS::deregister_wizard( PyObject* aPyWizard )
{
    // deregister also destroyes the previously created "PYTHON_FOOTPRINT_WIZARD object"
    FOOTPRINT_WIZARDS::deregister_object( (void*) aPyWizard );
}
