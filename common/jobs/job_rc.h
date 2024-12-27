/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <widgets/report_severity.h>
#include "job.h"

class KICOMMON_API JOB_RC : public JOB
{
public:
    JOB_RC( const std::string& aType );

    wxString m_filename;
    wxString m_outputFile;

    enum class UNITS
    {
        INCHES,
        MILLIMETERS,
        MILS
    };

    UNITS m_units;

    int m_severity;

    enum class OUTPUT_FORMAT
    {
        REPORT,
        JSON
    };

    OUTPUT_FORMAT m_format;

    bool m_exitCodeViolations;
};