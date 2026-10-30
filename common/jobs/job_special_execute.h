/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Mark Roszko <mark.roszko@gmail.com>
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

#pragma once

#include <map>
#include <vector>

#include <kicommon.h>
#include <jobs/job.h>

class PROJECT;

#define OUTPUT_TMP_PATH_VAR_NAME wxT( "JOBSET_OUTPUT_WORK_PATH" )

#define EXEC_PYTHON_VAR_NAME      wxT( "KICAD_PYTHON" )
#define EXEC_PROJECT_PCB_VAR_NAME wxT( "KICAD_PROJECT_PCB" )
#define EXEC_PROJECT_SCH_VAR_NAME wxT( "KICAD_PROJECT_SCH" )

/**
 * Find the interpreter shipped with KiCad in the given tool directories.
 *
 * @return the full path of the first interpreter found, or an empty string when none is shipped.
 */
KICOMMON_API wxString FindBundledPython( const std::vector<wxString>& aToolDirs );

/**
 * Return the variables an Execute Command job can use beyond the environment and project variables.
 *
 * KICAD_PYTHON is the bundled interpreter, or the bare interpreter name when KiCad ships none.
 * KICAD_PROJECT_PCB and KICAD_PROJECT_SCH are the project's board and top-level schematic. The
 * schematic comes from the project file. The board is named after the project, or is the only
 * board in the project folder.
 */
KICOMMON_API std::map<wxString, wxString> GetExecuteBuiltinVariables( const PROJECT* aProject );

/**
 * Describe each variable returned by GetExecuteBuiltinVariables() for the user.
 */
KICOMMON_API std::map<wxString, wxString> GetExecuteBuiltinVariableHelp();

/**
 * Expand an Execute Command string, including the built-in variables.
 *
 * A variable the user already defined, as an environment variable or a project text variable, wins
 * over the built-in of the same name so a portable jobset can still be overridden on one machine.
 */
KICOMMON_API wxString ExpandExecuteCommand( const wxString& aCommand, const PROJECT* aProject );

class KICOMMON_API JOB_SPECIAL_EXECUTE : public JOB
{
public:
    JOB_SPECIAL_EXECUTE();

    wxString GetDefaultDescription() const override;

    wxString m_command;

    bool     m_ignoreExitcode;
    bool     m_recordOutput;
};