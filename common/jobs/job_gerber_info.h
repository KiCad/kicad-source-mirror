/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#ifndef JOB_GERBER_INFO_H
#define JOB_GERBER_INFO_H

#include <kicommon.h>
#include "job.h"

/**
 * Job to extract information from Gerber/Excellon files.
 *
 * Outputs summary information including bounding box, layer count, aperture usage,
 * and copper area estimation.
 */
class KICOMMON_API JOB_GERBER_INFO : public JOB
{
public:
    JOB_GERBER_INFO();

    wxString GetDefaultDescription() const override;
    wxString GetSettingsDialogTitle() const override;

    enum class OUTPUT_FORMAT
    {
        TEXT,
        JSON
    };

    enum class UNITS
    {
        MM,
        INCH,
        MILS
    };

    wxString      m_inputFile;
    OUTPUT_FORMAT m_outputFormat = OUTPUT_FORMAT::TEXT;
    UNITS         m_units = UNITS::MM;
    bool          m_calculateArea = false;
    bool          m_strict = false;
};

#endif
