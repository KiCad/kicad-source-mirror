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

#include <jobs/job_export_sch_plot.h>
#include <jobs/job_registry.h>
#include <i18n_utility.h>

NLOHMANN_JSON_SERIALIZE_ENUM( JOB_PAGE_SIZE,
                              {
                                      { JOB_PAGE_SIZE::PAGE_SIZE_AUTO, "auto" },
                                      { JOB_PAGE_SIZE::PAGE_SIZE_A4, "A4" },
                                      { JOB_PAGE_SIZE::PAGE_SIZE_A, "A" },
                              } )

NLOHMANN_JSON_SERIALIZE_ENUM( JOB_HPGL_PAGE_SIZE,
                              {
                                      { JOB_HPGL_PAGE_SIZE::DEFAULT, "default" },
                                      { JOB_HPGL_PAGE_SIZE::SIZE_A5, "A5" },
                                      { JOB_HPGL_PAGE_SIZE::SIZE_A4, "A4" },
                                      { JOB_HPGL_PAGE_SIZE::SIZE_A3, "A3" },
                                      { JOB_HPGL_PAGE_SIZE::SIZE_A2, "A2" },
                                      { JOB_HPGL_PAGE_SIZE::SIZE_A1, "A1" },
                                      { JOB_HPGL_PAGE_SIZE::SIZE_A0, "A0" },
                                      { JOB_HPGL_PAGE_SIZE::SIZE_A, "A" },
                                      { JOB_HPGL_PAGE_SIZE::SIZE_B, "B" },
                                      { JOB_HPGL_PAGE_SIZE::SIZE_C, "C" },
                                      { JOB_HPGL_PAGE_SIZE::SIZE_D, "D" },
                                      { JOB_HPGL_PAGE_SIZE::SIZE_E, "E" },
                              } )

NLOHMANN_JSON_SERIALIZE_ENUM( JOB_HPGL_PLOT_ORIGIN_AND_UNITS,
                              {
                                      { JOB_HPGL_PLOT_ORIGIN_AND_UNITS::PLOTTER_BOT_LEFT, "default" },
                                      { JOB_HPGL_PLOT_ORIGIN_AND_UNITS::PLOTTER_CENTER, "A5" },
                                      { JOB_HPGL_PLOT_ORIGIN_AND_UNITS::USER_FIT_PAGE, "A4" },
                                      { JOB_HPGL_PLOT_ORIGIN_AND_UNITS::USER_FIT_CONTENT, "A3" },
                              } )

NLOHMANN_JSON_SERIALIZE_ENUM( SCH_PLOT_FORMAT,
                              {
                                      { SCH_PLOT_FORMAT::HPGL, "hpgl" },
                                      { SCH_PLOT_FORMAT::PDF, "pdf" },
                                      { SCH_PLOT_FORMAT::GERBER, "gerber" },
                                      { SCH_PLOT_FORMAT::POST, "post" },
                                      { SCH_PLOT_FORMAT::SVG, "svg" },
                                      { SCH_PLOT_FORMAT::DXF, "dxf" },
                              } )

JOB_EXPORT_SCH_PLOT::JOB_EXPORT_SCH_PLOT( bool aIsCli ) :
        JOB( "plot", false, aIsCli ),
        m_plotFormat( SCH_PLOT_FORMAT::PDF ),
        m_filename(),
        m_drawingSheet(),
        m_plotAll( true ),
        m_plotDrawingSheet( true ),
        m_blackAndWhite( false ),
        m_pageSizeSelect( JOB_PAGE_SIZE::PAGE_SIZE_AUTO ),
        m_useBackgroundColor( true ),
        m_HPGLPenSize( 1.0 ),
        m_HPGLPaperSizeSelect( JOB_HPGL_PAGE_SIZE::DEFAULT ),
        m_PDFPropertyPopups( true ),
        m_PDFHierarchicalLinks( true ),
        m_PDFMetadata( true ),
        m_theme(),
        m_outputDirectory(),
        m_outputFile(),
        m_HPGLPlotOrigin( JOB_HPGL_PLOT_ORIGIN_AND_UNITS::USER_FIT_CONTENT )
{
    m_params.emplace_back(
            new JOB_PARAM<SCH_PLOT_FORMAT>( "format", &m_plotFormat, m_plotFormat ) );
    m_params.emplace_back(
            new JOB_PARAM<wxString>( "drawing_sheet", &m_drawingSheet, m_drawingSheet ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "plot_all", &m_plotAll, m_plotAll ) );
    m_params.emplace_back(
            new JOB_PARAM<bool>( "plot_drawing_sheet", &m_plotDrawingSheet, m_plotDrawingSheet ) );
    m_params.emplace_back(
            new JOB_PARAM<bool>( "black_and_white", &m_blackAndWhite, m_blackAndWhite ) );
    m_params.emplace_back(
            new JOB_PARAM<JOB_PAGE_SIZE>( "page_size", &m_pageSizeSelect, m_pageSizeSelect ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "use_background_color", &m_useBackgroundColor,
                                                m_useBackgroundColor ) );
    m_params.emplace_back(
            new JOB_PARAM<double>( "hpgl_pen_size", &m_HPGLPenSize, m_HPGLPenSize ) );
    m_params.emplace_back( new JOB_PARAM<JOB_HPGL_PAGE_SIZE>(
            "hpgl_page_size", &m_HPGLPaperSizeSelect, m_HPGLPaperSizeSelect ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "pdf_property_popups", &m_PDFPropertyPopups,
                                                m_PDFPropertyPopups ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "pdf_hierarchical_links", &m_PDFHierarchicalLinks,
                                                m_PDFHierarchicalLinks ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "pdf_metadata", &m_PDFMetadata, m_PDFMetadata ) );
    m_params.emplace_back( new JOB_PARAM<wxString>( "color_theme", &m_theme, m_theme ) );
    m_params.emplace_back(
            new JOB_PARAM<wxString>( "output_filename", &m_outputFile, m_outputFile ) );
    m_params.emplace_back(
            new JOB_PARAM<wxString>( "output_directory", &m_outputDirectory, m_outputDirectory ) );
    m_params.emplace_back( new JOB_PARAM<JOB_HPGL_PLOT_ORIGIN_AND_UNITS>(
            "hpgl_plot_origin", &m_HPGLPlotOrigin, m_HPGLPlotOrigin ) );

}


wxString JOB_EXPORT_SCH_PLOT::GetDescription()
{
    return wxString::Format( _( "Schematic plot export" ) );
}


JOB_EXPORT_SCH_PLOT_PDF::JOB_EXPORT_SCH_PLOT_PDF( bool aIsCli ) :
		JOB_EXPORT_SCH_PLOT( aIsCli )
{
    m_plotFormat = SCH_PLOT_FORMAT::PDF;
}


JOB_EXPORT_SCH_PLOT_DXF ::JOB_EXPORT_SCH_PLOT_DXF ( bool aIsCli ) :
		JOB_EXPORT_SCH_PLOT( aIsCli )
{
	m_plotFormat = SCH_PLOT_FORMAT::DXF;
}


JOB_EXPORT_SCH_PLOT_SVG::JOB_EXPORT_SCH_PLOT_SVG( bool aIsCli ) :
		JOB_EXPORT_SCH_PLOT( aIsCli )
{
	m_plotFormat = SCH_PLOT_FORMAT::SVG;
}


JOB_EXPORT_SCH_PLOT_PS::JOB_EXPORT_SCH_PLOT_PS( bool aIsCli ) :
		JOB_EXPORT_SCH_PLOT( aIsCli )
{
	m_plotFormat = SCH_PLOT_FORMAT::POST;
}


JOB_EXPORT_SCH_PLOT_HPGL::JOB_EXPORT_SCH_PLOT_HPGL( bool aIsCli ) :
		JOB_EXPORT_SCH_PLOT( aIsCli )
{
	m_plotFormat = SCH_PLOT_FORMAT::HPGL;
}


REGISTER_JOB( sch_export_plot_svg, _HKI( "Schematic: Export SVG" ), KIWAY::FACE_SCH,
              JOB_EXPORT_SCH_PLOT_SVG );
REGISTER_JOB( sch_export_plot_hpgl, _HKI( "Schematic: Export HPGL" ), KIWAY::FACE_SCH,
              JOB_EXPORT_SCH_PLOT_HPGL );
REGISTER_JOB( sch_export_plot_ps, _HKI( "Schematic: Export PS" ), KIWAY::FACE_SCH,
              JOB_EXPORT_SCH_PLOT_PS );
REGISTER_JOB( sch_export_plot_dxf, _HKI( "Schematic: Export DXF" ), KIWAY::FACE_SCH,
              JOB_EXPORT_SCH_PLOT_DXF );
REGISTER_JOB( sch_export_plot_pdf, _HKI( "Schematic: Export PDF" ), KIWAY::FACE_SCH,
              JOB_EXPORT_SCH_PLOT_PDF );