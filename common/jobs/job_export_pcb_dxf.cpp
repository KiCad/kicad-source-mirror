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

#include <jobs/job_export_pcb_dxf.h>
#include <jobs/job_registry.h>
#include <i18n_utility.h>

NLOHMANN_JSON_SERIALIZE_ENUM( JOB_EXPORT_PCB_DXF::GEN_MODE,
                              {
                                      { JOB_EXPORT_PCB_DXF::GEN_MODE::MULTI, "multi" },
                                      { JOB_EXPORT_PCB_DXF::GEN_MODE::SINGLE, "single" },
                              } )

NLOHMANN_JSON_SERIALIZE_ENUM( JOB_EXPORT_PCB_DXF::DXF_UNITS,
                              {
                                      { JOB_EXPORT_PCB_DXF::DXF_UNITS::INCH, "in" },
                                      { JOB_EXPORT_PCB_DXF::DXF_UNITS::MM, "mm" },
                              } )

JOB_EXPORT_PCB_DXF::JOB_EXPORT_PCB_DXF() :
        JOB_EXPORT_PCB_PLOT( JOB_EXPORT_PCB_PLOT::PLOT_FORMAT::DXF, "dxf", false ),
        m_plotGraphicItemsUsingContours( true ),
        m_polygonMode( true ),
        m_dxfUnits( DXF_UNITS::INCH ),
        m_genMode( GEN_MODE::MULTI )
{
    m_plotDrawingSheet = false;

    m_params.emplace_back( new JOB_PARAM<double>( "scale", &m_scale, m_scale ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "plot_graphic_items_using_contours",
                                                &m_plotGraphicItemsUsingContours,
                                                m_plotGraphicItemsUsingContours ) );
    m_params.emplace_back( new JOB_PARAM<DXF_UNITS>( "units", &m_dxfUnits, m_dxfUnits ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "polygon_mode", &m_polygonMode, m_polygonMode ) );
    m_params.emplace_back( new JOB_PARAM<GEN_MODE>( "gen_mode", &m_genMode, m_genMode ) );
}


wxString JOB_EXPORT_PCB_DXF::GetDefaultDescription() const
{
    return _( "Export DXF" );
}


wxString JOB_EXPORT_PCB_DXF::GetSettingsDialogTitle() const
{
    return _( "Export DXF Job Settings" );
}


REGISTER_JOB( pcb_export_dxf, _HKI( "PCB: Export DXF" ), KIWAY::FACE_PCB, JOB_EXPORT_PCB_DXF );