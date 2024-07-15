/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Mark Roszko <mark.roszko@gmail.com>
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

#ifndef JOB_PCB_DRC_H
#define JOB_PCB_DRC_H

#include <kicommon.h>
#include <layer_ids.h>
#include <wx/string.h>
#include <widgets/report_severity.h>
#include "job.h"

class KICOMMON_API JOB_PCB_DRC : public JOB
{
public:
    JOB_PCB_DRC( bool aIsCli );
    wxString GetDescription() override;

    wxString m_filename;
    wxString m_outputFile;

    bool m_reportAllTrackErrors;

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
    bool m_parity;
};

#endif
