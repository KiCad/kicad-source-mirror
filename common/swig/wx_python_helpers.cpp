/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Miguel Angel Ajo <miguelangel@nbee.es>
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
 * @file wx_python_helpers.cpp
 * @brief Python wrapping helpers for wx structures/objects
 */

#include <Python.h> // must be first to avoid wx/python typedef conflicts on msvc
#include <string_utils.h>
#include <wx/intl.h>
#include <wx/string.h>
#include <wx/arrstr.h>


#define WX_DEFAULTENCODING_SIZE 64

static char wxPythonEncoding[WX_DEFAULTENCODING_SIZE] = "ascii";


PyObject* wxArrayString2PyList( const wxArrayString& lst )
{
    PyObject* list = PyList_New( 0 );

    for( size_t i = 0; i < lst.GetCount(); i++ )
    {
        PyObject* pyStr = PyUnicode_FromString( lst[i].utf8_str() );
        PyList_Append( list, pyStr );
        Py_DECREF( pyStr );
    }

    return list;
}


wxString Py2wxString( PyObject* src )
{
    bool        must_unref_str = false;
    bool        must_unref_obj = false;

    wxString    result;

    PyObject*   obj     = src;
    PyObject*   uni_str = src;

    // if not an str or unicode, try to str(src)
    if( !PyBytes_Check( src ) && !PyUnicode_Check( src ) )
    {
        obj = PyObject_Str( src );
        uni_str = obj; // in case of Python 3 our string is already correctly encoded
        must_unref_obj = true;

        if( PyErr_Occurred() )
            return result;
    }

    if( PyBytes_Check( obj ) )
    {
        uni_str = PyUnicode_FromEncodedObject( obj, wxPythonEncoding, "strict" );
        must_unref_str = true;

        if( PyErr_Occurred() )
            return result;
    }

    size_t len = PyUnicode_GET_LENGTH( uni_str );

    if( len )
        result = From_UTF8( PyUnicode_AsUTF8( uni_str ) );

    if( must_unref_str )
    {
        Py_DECREF( uni_str );
    }

    if( must_unref_obj )
    {
        Py_DECREF( obj );
    }


    return result;
}


PyObject* wx2PyString( const wxString& src )
{
    return PyUnicode_FromString( src.utf8_str() );
}


void wxSetDefaultPyEncoding( const char* encoding )
{
    strncpy( wxPythonEncoding, encoding, WX_DEFAULTENCODING_SIZE );
    wxPythonEncoding[ WX_DEFAULTENCODING_SIZE - 1 ] = '\0';
}


const char* wxGetDefaultPyEncoding()
{
    return wxPythonEncoding;
}
