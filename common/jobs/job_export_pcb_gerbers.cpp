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

#include <jobs/job_export_pcb_gerbers.h>
#include <jobs/lset_json.h>
#include <jobs/job_registry.h>
#include <i18n_utility.h>

JOB_EXPORT_PCB_GERBERS::JOB_EXPORT_PCB_GERBERS() :
        JOB_EXPORT_PCB_GERBER( "gerbers" ),
        m_layersIncludeOnAll(),
        m_layersIncludeOnAllSet( false ),
        m_useBoardPlotParams( false ),
        m_createJobsFile( true )
{
    m_params.emplace_back( new JOB_PARAM<bool>( "use_board_plot_params", &m_useBoardPlotParams,
                                                m_useBoardPlotParams ) );

    m_params.emplace_back( new JOB_PARAM<bool>( "layers_include_on_all_set", &m_layersIncludeOnAllSet,
                                                m_layersIncludeOnAllSet ) );

    m_params.emplace_back( new JOB_PARAM<LSET>( "layers_include_on_all", &m_layersIncludeOnAll,
                                                m_layersIncludeOnAll ) );
}


wxString JOB_EXPORT_PCB_GERBERS::GetDefaultDescription() const
{
    return wxString::Format( _( "Multi gerber export" ) );
}


REGISTER_JOB( pcb_export_gerbers, _HKI( "PCB: Export Gerbers" ), KIWAY::FACE_PCB,
              JOB_EXPORT_PCB_GERBERS );