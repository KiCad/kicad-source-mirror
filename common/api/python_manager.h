/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Jon Evans <jon@craftyjon.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KICAD_PYTHON_MANAGER_H
#define KICAD_PYTHON_MANAGER_H

#include <functional>
#include <optional>

#include <wx/wx.h>

#include <kicommon.h>


class KICOMMON_API PYTHON_MANAGER
{
public:
    PYTHON_MANAGER( const wxString& aInterpreterPath );

    /**
     * Launches the Python interpreter with the given arguments
     * @param aArgs
     * @param aCallback
     * @param aEnv
     * @param aSaveOutput
     * @return the process ID of the created process, or 0 if one was not created
     */
    long Execute( const std::vector<wxString>& aArgs,
                  const std::function<void(int, const wxString&, const wxString&)>& aCallback,
                  const wxExecuteEnv* aEnv = nullptr,
                  bool aSaveOutput = false );

    long ExecuteSync( const std::vector<wxString>& aArgs,
                      wxString* aStdout = nullptr, wxString* aStderr = nullptr,
                      const wxExecuteEnv* aEnv = nullptr );

    wxString GetInterpreterPath() const { return m_interpreterPath; }
    void SetInterpreterPath( const wxString& aPath ) { m_interpreterPath = aPath; }

    /**
     * Searches for a Python intepreter on the user's system
     * @return the absolute path to a Python interpreter, or an empty string if one was not found
     */
    static wxString FindPythonInterpreter();

    static std::optional<wxString> GetPythonEnvironment( const wxString& aNamespace );

    /// Returns a full path to the python binary in a venv, if it exists
    static std::optional<wxString> GetVirtualPython( const wxString& aNamespace );

    /**
     * Returns the path of the interpreter that was used to create the venv in the given
     * environment directory according to its config file.  May return nullopt if the venv
     * is invalid but also if it was created with an older Python version (venv only started
     * recording the executable path in Python 3.11)
     */
    static std::optional<wxString> GetVenvInterpreter( const wxString& aEnvPath );

    static bool IsVenvUsable( const wxString& aEnvPath );

    /**
     * Returns true if the venv at aEnvPath was created by an interpreter other than
     * aConfiguredInterpreter.
     */
    static bool IsVenvStale( const wxString& aEnvPath, const wxString& aConfiguredInterpreter );

private:
    wxString m_interpreterPath;
};


#endif //KICAD_PYTHON_MANAGER_H
