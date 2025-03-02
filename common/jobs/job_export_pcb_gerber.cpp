/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Mark Roszko <mark.roszko@gmail.com>
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

#include <jobs/job_export_pcb_gerber.h>
#include <jobs/job_registry.h>
#include <i18n_utility.h>


JOB_EXPORT_PCB_GERBER::JOB_EXPORT_PCB_GERBER( const std::string& aType ) :
    JOB_EXPORT_PCB_PLOT( JOB_EXPORT_PCB_PLOT::PLOT_FORMAT::GERBER, aType, false ),
    m_includeNetlistAttributes( true ),
    m_useX2Format( true ),
    m_disableApertureMacros( false ),
    m_useProtelFileExtension( true ),
    m_precision( 5 )
{
    m_plotDrawingSheet = false;

    m_params.emplace_back( new JOB_PARAM<bool>( "include_netlist_attributes",
                                                &m_includeNetlistAttributes,
                                                m_includeNetlistAttributes ) );

    m_params.emplace_back( new JOB_PARAM<bool>( "use_x2_format", &m_useX2Format, m_useX2Format ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "disable_aperture_macros",
                                                &m_disableApertureMacros,
                                                m_disableApertureMacros ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "use_protel_file_extension",
                                                &m_useProtelFileExtension,
                                                m_useProtelFileExtension ) );
    m_params.emplace_back( new JOB_PARAM<int>( "precision", &m_precision, m_precision ) );
}


JOB_EXPORT_PCB_GERBER::JOB_EXPORT_PCB_GERBER() :
    JOB_EXPORT_PCB_GERBER( "gerber" )
{
}


wxString JOB_EXPORT_PCB_GERBER::GetDefaultDescription() const
{
    return _( "Export single Gerber" );
}


wxString JOB_EXPORT_PCB_GERBER::GetSettingsDialogTitle() const
{
    return _( "Export Single Gerber Job Settings" );
}