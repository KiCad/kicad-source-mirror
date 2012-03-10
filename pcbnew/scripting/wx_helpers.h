#ifndef __wx_helpers_h
#define __wx_helpers_h

#include <Python.h>
#include <wx/intl.h>
#include <wx/string.h>

wxString* newWxStringFromPy(PyObject* source);
wxString Py2wxString(PyObject* source);
PyObject* wx2PyString(const wxString& src);

void wxSetDefaultPyEncoding(const char* encoding);
const char* wxGetDefaultPyEncoding();

#endif
