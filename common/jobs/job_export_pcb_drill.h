/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
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

#ifndef JOB_EXPORT_PCB_DRILL_H
#define JOB_EXPORT_PCB_DRILL_H

#include <kicommon.h>
#include <layer_ids.h>
#include "job.h"

class KICOMMON_API JOB_EXPORT_PCB_DRILL : public JOB
{
public:
    JOB_EXPORT_PCB_DRILL();

    wxString GetDefaultDescription() const override;
    wxString GetSettingsDialogTitle() const override;

    enum class DRILL_FORMAT
    {
        EXCELLON,
        GERBER
    };

    enum class DRILL_ORIGIN
    {
        ABS,
        PLOT
    };

    enum class DRILL_UNITS
    {
        INCH,       // Do not use IN: it conflicts with a Windows header
        MM
    };

    enum class ZEROS_FORMAT
    {
        DECIMAL,
        SUPPRESS_LEADING,
        SUPPRESS_TRAILING,
        KEEP_ZEROS
    };

    enum class MAP_FORMAT
    {
        POSTSCRIPT,
        GERBER_X2,
        DXF,
        SVG,
        PDF
    };

public:
    wxString     m_filename;

    bool         m_excellonMirrorY;
    bool         m_excellonMinimalHeader;
    bool         m_excellonCombinePTHNPTH;
    bool         m_excellonOvalDrillRoute;

    DRILL_FORMAT m_format;
    DRILL_ORIGIN m_drillOrigin;
    DRILL_UNITS  m_drillUnits;
    ZEROS_FORMAT m_zeroFormat;

    MAP_FORMAT   m_mapFormat;
    int          m_gerberPrecision;
    bool         m_generateMap;

    bool         m_generateTenting;

    bool         m_generateReport;
    wxString     m_reportPath;
};

#endif
