#ifndef WX_HELPERS_H_
#define WX_HELPERS_H_

#include <Python.h>
#include <wx/intl.h>
#include <wx/string.h>
#include <wx/arrstr.h>


PyObject*   wxArrayString2PyList( const wxArrayString& lst );
wxString    Py2wxString( PyObject* source );
PyObject*   wx2PyString( const wxString& src );

void        wxSetDefaultPyEncoding( const char* encoding );
const char* wxGetDefaultPyEncoding();

#endif  // WX_HELPERS_H_
