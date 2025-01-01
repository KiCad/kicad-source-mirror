/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#ifndef __PYTHON_SCRIPTING_H
#define __PYTHON_SCRIPTING_H

// undefs explained here: https://bugzilla.redhat.com/show_bug.cgi?id=427617

#ifdef _POSIX_C_SOURCE
    #undef _POSIX_C_SOURCE
#endif
#ifdef _XOPEN_SOURCE
    #undef _XOPEN_SOURCE
#endif

#if defined(WIN32)
    #undef pid_t    // wxWidgets defines this, python typedefs, result is a conflict
#endif

#undef HAVE_CLOCK_GETTIME  // macro is defined in Python.h and causes redefine warning
#include <Python.h>
#undef HAVE_CLOCK_GETTIME

#include <wx/window.h>
#include <wx/string.h>
#include <wx/arrstr.h>

#include <kicommon.h>

class KICOMMON_API SCRIPTING
{
public:
    SCRIPTING();
    ~SCRIPTING();

    /// We do not allow secondary creation of the scripting system
    SCRIPTING( SCRIPTING const& )       = delete;
    void operator= ( SCRIPTING const& ) = delete;

    static bool IsWxAvailable();

    static bool IsModuleLoaded( std::string& aModule );

    enum PATH_TYPE
    {
        STOCK,
        USER,
        THIRDPARTY
    };

    static wxString PyScriptingPath( PATH_TYPE aPathType = STOCK );
    static wxString PyPluginsPath( PATH_TYPE aPathType = STOCK );

private:

    bool scriptingSetup();

    PyThreadState* m_python_thread_state;
};

/**
 * Set an environment variable in the current Python interpreter.
 *
 * @param aVar is the variable to set
 * @param aValue is the value to give it
 */
KICOMMON_API void UpdatePythonEnvVar( const wxString& aVar, const wxString& aValue );

KICOMMON_API void RedirectStdio();
KICOMMON_API wxWindow* CreatePythonShellWindow( wxWindow* parent, const wxString& aFramenameId );
KICOMMON_API bool       InitPythonScripting( const char* aStockScriptingPath,
                                            const char* aUserScriptingPath );
KICOMMON_API bool       IsWxPythonLoaded();

class KICOMMON_API PyLOCK
{
    PyGILState_STATE gil_state;
public:
    PyLOCK()      { gil_state = PyGILState_Ensure(); }
    ~PyLOCK()     { PyGILState_Release( gil_state ); }
};

KICOMMON_API wxString PyStringToWx( PyObject* str );
KICOMMON_API wxArrayString PyArrayStringToWx( PyObject* arr );
KICOMMON_API wxString       PyErrStringWithTraceback();

#endif    // __PYTHON_SCRIPTING_H
