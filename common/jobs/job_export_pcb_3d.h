/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 1992-2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <kicommon.h>
#include <wx/string.h>
#include "job.h"

class KICOMMON_API JOB_EXPORT_PCB_3D : public JOB
{
public:
    JOB_EXPORT_PCB_3D( bool aIsCli );

    enum class FORMAT
    {
        UNKNOWN, // defefer to arg
        STEP,
        BREP,
        GLB,
        VRML
    };

    enum class VRML_UNITS
    {
        INCHES,
        MILLIMETERS,
        METERS,
        TENTHS // inches
    };

    bool                      m_overwrite;
    bool                      m_useGridOrigin;
    bool                      m_useDrillOrigin;
    bool                      m_hasUserOrigin;
    bool                      m_boardOnly;
    bool                      m_includeUnspecified;
    bool                      m_includeDNP;
    bool                      m_substModels;
    bool                      m_optimizeStep;
    wxString                  m_filename;
    wxString                  m_outputFile;
    double                    m_xOrigin;
    double                    m_yOrigin;
    double                    m_BoardOutlinesChainingEpsilon;
    bool                      m_exportBoardBody;
    bool                      m_exportComponents;
    bool                      m_exportTracks;
    bool                      m_exportZones;
    bool                      m_exportInnerCopper;
    bool                      m_fuseShapes;
    JOB_EXPORT_PCB_3D::FORMAT m_format;

    VRML_UNITS m_vrmlUnits;
    wxString   m_vrmlModelDir;
    bool       m_vrmlRelativePaths;
};

#endif
