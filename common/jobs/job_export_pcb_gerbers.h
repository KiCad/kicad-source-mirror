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

#ifndef JOB_EXPORT_PCB_GERBERS_H
#define JOB_EXPORT_PCB_GERBERS_H

#include "job_export_pcb_gerber.h"
#include <layer_ids.h>
#include <wx/string.h>
#include "job.h"

class JOB_EXPORT_PCB_GERBERS : public JOB_EXPORT_PCB_GERBER
{
public:
    JOB_EXPORT_PCB_GERBERS( bool aIsCli ) :
            JOB_EXPORT_PCB_GERBER( "gerbers", aIsCli ),
            m_layersIncludeOnAll(),
            m_layersIncludeOnAllSet( false ),
            m_useBoardPlotParams( false )
    {
    }

    LSET m_layersIncludeOnAll;

    bool m_layersIncludeOnAllSet;
    bool m_useBoardPlotParams;
};

#endif