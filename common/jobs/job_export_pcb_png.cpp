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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <jobs/job_export_pcb_png.h>
#include <jobs/job_registry.h>
#include <i18n_utility.h>
#include <plotters/plotter_png.h>

JOB_EXPORT_PCB_PNG::JOB_EXPORT_PCB_PNG() :
    JOB_EXPORT_PCB_PLOT( JOB_EXPORT_PCB_PLOT::PLOT_FORMAT::PNG, "png", false ),
    m_dpi( DEFAULT_PNG_DPI ),
    m_antialias( true )
{
    m_params.emplace_back( new JOB_PARAM<int>( "dpi", &m_dpi, m_dpi ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "antialias", &m_antialias, m_antialias ) );
}


wxString JOB_EXPORT_PCB_PNG::GetDefaultDescription() const
{
    return _( "Export PNG" );
}


wxString JOB_EXPORT_PCB_PNG::GetSettingsDialogTitle() const
{
    return _( "Export PNG Job Settings" );
}


REGISTER_JOB( pcb_export_png, _HKI( "PCB: Export PNG" ), KIWAY::FACE_PCB, JOB_EXPORT_PCB_PNG );
