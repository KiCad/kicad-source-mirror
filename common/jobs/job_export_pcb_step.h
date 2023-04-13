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

#ifndef JOB_EXPORT_STEP_H
#define JOB_EXPORT_STEP_H

#include <wx/string.h>
#include "job.h"

class JOB_EXPORT_PCB_STEP : public JOB
{
public:
    JOB_EXPORT_PCB_STEP( bool aIsCli ) :
            JOB( "step", aIsCli ),
            m_overwrite( false ),
            m_useGridOrigin( false ),
            m_useDrillOrigin( false ),
            m_boardOnly( false ),
            m_includeUnspecified( false ),
            m_includeDNP( false ),
            m_substModels( false ),
            m_filename(),
            m_outputFile(),
            m_xOrigin( 0.0 ),
            m_yOrigin( 0.0 ),
            // max dist to chain 2 items (lines or curves) to build the board outlines
            m_BoardOutlinesChainingEpsilon( 0.01 ),     // 0.01 mm is a good value
            m_exportTracks( false )     // Extremely time consuming if true
    {
    }

    bool     m_overwrite;
    bool     m_useGridOrigin;
    bool     m_useDrillOrigin;
    bool     m_boardOnly;
    bool     m_includeUnspecified;
    bool     m_includeDNP;
    bool     m_substModels;
    wxString m_filename;
    wxString m_outputFile;
    double   m_xOrigin;
    double   m_yOrigin;
    double   m_BoardOutlinesChainingEpsilon;
    bool     m_exportTracks;
};

#endif
