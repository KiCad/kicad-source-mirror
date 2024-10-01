/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 1992-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef JOB_EXPORT_PCB_DRILL_H
#define JOB_EXPORT_PCB_DRILL_H

#include <kicommon.h>
#include <layer_ids.h>
#include <wx/string.h>
#include "job.h"

class KICOMMON_API JOB_EXPORT_PCB_DRILL : public JOB
{
public:
    JOB_EXPORT_PCB_DRILL( bool aIsCli );

    wxString GetDescription() override;

    wxString m_filename;

    bool m_excellonMirrorY;
    bool m_excellonMinimalHeader;
    bool m_excellonCombinePTHNPTH;
    bool m_excellonOvalDrillRoute;

    enum class DRILL_FORMAT
    {
        EXCELLON,
        GERBER
    };

    DRILL_FORMAT m_format;

    enum class DRILL_ORIGIN
    {
        ABS,
        PLOT
    };

    DRILL_ORIGIN m_drillOrigin;

    enum class DRILL_UNITS
    {
        INCHES,
        MILLIMETERS
    };

    DRILL_UNITS m_drillUnits;

    enum class ZEROS_FORMAT
    {
        DECIMAL,
        SUPPRESS_LEADING,
        SUPPRESS_TRAILING,
        KEEP_ZEROS
    };

    ZEROS_FORMAT m_zeroFormat;

    enum class MAP_FORMAT
    {
        POSTSCRIPT,
        GERBER_X2,
        DXF,
        SVG,
        PDF
    };

    MAP_FORMAT m_mapFormat;

    int m_gerberPrecision;

    bool m_generateMap;
};

#endif
