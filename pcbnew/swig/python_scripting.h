#ifndef __PYTHON_SCRIPTING_H
#define __PYTHON_SCRIPTING_H

// undefs explained here: https://bugzilla.redhat.com/show_bug.cgi?id=427617

#ifdef _POSIX_C_SOURCE
    #undef _POSIX_C_SOURCE
#endif
#ifdef _XOPEN_SOURCE
    #undef _XOPEN_SOURCE
#endif

#include <Python.h>
#ifndef NO_WXPYTHON_EXTENSION_HEADERS
#ifdef KICAD_SCRIPTING_WXPYTHON
    #include <wx/wxPython/wxPython.h>
#endif
#endif

#include <wx/string.h>
#include <wx/arrstr.h>

/* Function pcbnewInitPythonScripting
 * Initializes the Python engine inside pcbnew
 */

bool        pcbnewInitPythonScripting( const char * aUserScriptingPath );
void        pcbnewFinishPythonScripting();


#ifdef KICAD_SCRIPTING_WXPYTHON

void        RedirectStdio();
wxWindow*   CreatePythonShellWindow( wxWindow* parent, const wxString& aFramenameId );

class PyLOCK
{
    wxPyBlock_t b;
public:

    // @todo, find out why these are wxPython specific.  We need the GIL regardless.
    // Should never assume python will only have one thread calling it.
    PyLOCK()    { b = wxPyBeginBlockThreads(); }
    ~PyLOCK()   { wxPyEndBlockThreads( b ); }
};


#else
class PyLOCK
{
    PyGILState_STATE gil_state;
public:
    PyLOCK()      { gil_state = PyGILState_Ensure(); }
    ~PyLOCK()     { PyGILState_Release( gil_state ); }
};

#endif

wxArrayString   PyArrayStringToWx( PyObject* arr );
wxString        PyErrStringWithTraceback();

#endif    // __PYTHON_SCRIPTING_H
