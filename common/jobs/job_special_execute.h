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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <kicommon.h>
#include <wx/string.h>
#include <jobs/job.h>

#define OUTPUT_WORK_PATH_VAR_NAME wxT( "JOBSET_OUTPUT_WORK_PATH" )

class KICOMMON_API JOB_SPECIAL_EXECUTE : public JOB
{
public:
    JOB_SPECIAL_EXECUTE();

    wxString GetDefaultDescription() const override;

    wxString m_command;

    bool     m_ignoreExitcode;
    bool     m_recordOutput;
};