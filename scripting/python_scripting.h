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


/* Function pcbnewInitPythonScripting
 * Initializes the Python engine inside pcbnew 
 */

bool pcbnewInitPythonScripting();
void pcbnewFinishPythonScripting();



#ifdef KICAD_SCRIPTING_WXPYTHON

void RedirectStdio(); 
wxWindow* CreatePythonShellWindow(wxWindow* parent);

#define PY_BLOCK_THREADS(name)    wxPyBlock_t name = wxPyBeginBlockThreads()
#define PY_UNBLOCK_THREADS(name)    wxPyEndBlockThreads(name)

#else

#define PY_BLOCK_THREADS(name)    
#define PY_UNBLOCK_THREADS(name) 

#endif

#endif
