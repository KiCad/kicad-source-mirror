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

#ifndef JOB_EXPORT_SCH_PLOT_H
#define JOB_EXPORT_SCH_PLOT_H

#include <wx/string.h>
#include <plotters/plotter.h>
#include "job.h"

class JOB_EXPORT_SCH_PLOT : public JOB
{
public:
    JOB_EXPORT_SCH_PLOT( bool aIsCli, PLOT_FORMAT aPlotFormat, wxString aFilename ) :
            JOB( "plot", aIsCli ), m_plotFormat( aPlotFormat ), m_filename( aFilename )
    {
    }

    PLOT_FORMAT       m_plotFormat;
    wxString          m_filename;
    SCH_PLOT_SETTINGS settings;
};

#endif