/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <jobs/job_pcb_drc.h>
#include <jobs/job_registry.h>
#include <i18n_utility.h>

JOB_PCB_DRC::JOB_PCB_DRC() :
    JOB_RC( "drc" ),
    m_reportAllTrackErrors( false ),
    m_parity( true )
{
    m_params.emplace_back( new JOB_PARAM<bool>( "parity", &m_parity, m_parity ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "report_all_track_errors", &m_reportAllTrackErrors, m_reportAllTrackErrors ) );
}


wxString JOB_PCB_DRC::GetDefaultDescription() const
{
    return wxString::Format( _( "Perform PCB DRC" ) );
}


REGISTER_JOB( pcb_drc, _HKI( "PCB: Perform DRC" ), KIWAY::FACE_PCB, JOB_PCB_DRC );