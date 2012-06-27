#ifndef __wx_helpers_h
#define __wx_helpers_h

#include <Python.h>
#include <wx/intl.h>
#include <wx/string.h>
#include <wx/arrstr.h>


PyObject*   wxArrayString2PyList( const wxArrayString& lst );
wxString*   newWxStringFromPy( PyObject* source );
wxString    Py2wxString( PyObject* source );
PyObject*   wx2PyString( const wxString& src );

void        wxSetDefaultPyEncoding( const char* encoding );
const char* wxGetDefaultPyEncoding();

#endif
