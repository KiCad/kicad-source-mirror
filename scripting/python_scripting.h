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
        #include <wx/wxPython/wxPython.h>
    #endif


/* Function pcbnewInitPythonScripting
 * Initializes the Python engine inside pcbnew 
 */

bool pcbnewInitPythonScripting();
void pcbnewFinishPythonScripting();

#endif
