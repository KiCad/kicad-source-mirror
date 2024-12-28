/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 2023-2024 KiCad Developers, see AUTHORS.txt for contributors.
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

NLOHMANN_JSON_SERIALIZE_ENUM( JOB_EXPORT_PCB_DXF::DXF_UNITS,
                              {
                                      { JOB_EXPORT_PCB_DXF::DXF_UNITS::INCHES, "in" },
                                      { JOB_EXPORT_PCB_DXF::DXF_UNITS::MILLIMETERS, "mm" },
                              } )

JOB_EXPORT_PCB_DXF::JOB_EXPORT_PCB_DXF() :
    JOB_EXPORT_PCB_PLOT( JOB_EXPORT_PCB_PLOT::PLOT_FORMAT::DXF, "dxf", false ),
    m_plotGraphicItemsUsingContours( true ),
    m_dxfUnits( DXF_UNITS::INCHES )
{
    m_plotDrawingSheet = false;

    m_params.emplace_back(
            new JOB_PARAM<wxString>( "drawing_sheet", &m_drawingSheet, m_drawingSheet ) );
    m_params.emplace_back(
            new JOB_PARAM<bool>( "plot_footprint_values", &m_plotFootprintValues, m_plotFootprintValues ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "plot_ref_des", &m_plotRefDes, m_plotRefDes ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "plot_graphic_items_using_contours", &m_plotGraphicItemsUsingContours,
                                                m_plotGraphicItemsUsingContours ) );
    m_params.emplace_back(
            new JOB_PARAM<bool>( "use_drill_origin", &m_useDrillOrigin, m_useDrillOrigin ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "plot_border_title_blocks", &m_plotDrawingSheet,
                                                m_plotDrawingSheet ) );
    m_params.emplace_back( new JOB_PARAM<DXF_UNITS>( "units", &m_dxfUnits, m_dxfUnits ) );
    m_params.emplace_back(
            new JOB_PARAM<LSEQ>( "layers", &m_printMaskLayer, m_printMaskLayer ) );
}


wxString JOB_EXPORT_PCB_DXF::GetDescription()
{
    return wxString::Format( _( "PCB DXF export" ) );
}

REGISTER_JOB( pcb_export_dxf, _HKI( "PCB: Export DXF" ), KIWAY::FACE_PCB, JOB_EXPORT_PCB_DXF );