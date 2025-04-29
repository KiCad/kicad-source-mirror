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

#include <jobs/job_export_pcb_svg.h>
#include <jobs/job_registry.h>
#include <i18n_utility.h>

NLOHMANN_JSON_SERIALIZE_ENUM( JOB_EXPORT_PCB_SVG::GEN_MODE,
                              {
                                { JOB_EXPORT_PCB_SVG::GEN_MODE::MULTI, "multi" },   // intended gui behavior, first as default
                                { JOB_EXPORT_PCB_SVG::GEN_MODE::SINGLE, "single" },
                              } )

JOB_EXPORT_PCB_SVG::JOB_EXPORT_PCB_SVG() :
    JOB_EXPORT_PCB_PLOT( JOB_EXPORT_PCB_PLOT::PLOT_FORMAT::SVG, "svg", false ),
    m_fitPageToBoard( false ),
    m_precision( 4 ),
    m_genMode( GEN_MODE::SINGLE )   // TODO change to MULTI for V10
{
    m_plotDrawingSheet = true;

    m_params.emplace_back( new JOB_PARAM<double>( "scale", &m_scale, m_scale ) );
    m_params.emplace_back( new JOB_PARAM<wxString>( "color_theme", &m_colorTheme, m_colorTheme ) );
    m_params.emplace_back(
            new JOB_PARAM<bool>( "fit_page_to_board", &m_fitPageToBoard, m_fitPageToBoard ) );
    m_params.emplace_back( new JOB_PARAM<unsigned int>( "precision", &m_precision, m_precision ) );
    m_params.emplace_back( new JOB_PARAM<GEN_MODE>( "gen_mode", &m_genMode, m_genMode ) );
}


wxString JOB_EXPORT_PCB_SVG::GetDefaultDescription() const
{
    return _( "Export SVG" );
}


wxString JOB_EXPORT_PCB_SVG::GetSettingsDialogTitle() const
{
    return _( "Export SVG Job Settings" );
}


REGISTER_JOB( pcb_export_svg, _HKI( "PCB: Export SVG" ), KIWAY::FACE_PCB, JOB_EXPORT_PCB_SVG );
