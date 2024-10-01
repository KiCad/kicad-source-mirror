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

#include <jobs/job_export_pcb_svg.h>
#include <jobs/job_registry.h>
#include <i18n_utility.h>


JOB_EXPORT_PCB_SVG::JOB_EXPORT_PCB_SVG( bool aIsCli ) :
    JOB_EXPORT_PCB_PLOT( JOB_EXPORT_PCB_PLOT::PLOT_FORMAT::SVG, "svg", false, aIsCli ),
    m_pageSizeMode( 0 )
{
    m_plotDrawingSheet = true;

    m_params.emplace_back( new JOB_PARAM<wxString>( "color_theme", &m_colorTheme, m_colorTheme ) );
    m_params.emplace_back(
            new JOB_PARAM<wxString>( "drawing_sheet", &m_drawingSheet, m_drawingSheet ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "mirror", &m_mirror, m_mirror ) );
    m_params.emplace_back(
            new JOB_PARAM<bool>( "black_and_white", &m_blackAndWhite, m_blackAndWhite ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "negative", &m_negative, m_negative ) );
    m_params.emplace_back(
            new JOB_PARAM<bool>( "plot_drawing_sheet", &m_plotDrawingSheet, m_plotDrawingSheet ) );
    m_params.emplace_back( new JOB_PARAM<LSEQ>( "layers", &m_printMaskLayer, m_printMaskLayer ) );
    m_params.emplace_back( new JOB_PARAM<bool>(
            "sketch_pads_on_fab_layers", &m_sketchPadsOnFabLayers, m_sketchPadsOnFabLayers ) );
    m_params.emplace_back(
            new JOB_PARAM<int>( "page_size_mode", &m_pageSizeMode, m_pageSizeMode ) );
    m_params.emplace_back(
            new JOB_PARAM<int>( "drill_shape_option", &m_drillShapeOption, m_drillShapeOption ) );
}


wxString JOB_EXPORT_PCB_SVG::GetDescription()
{
    return wxString::Format( _( "PCB SVG export" ) );
}


REGISTER_JOB( pcb_export_svg, _HKI( "PCB: Export SVG" ), KIWAY::FACE_PCB, JOB_EXPORT_PCB_SVG );