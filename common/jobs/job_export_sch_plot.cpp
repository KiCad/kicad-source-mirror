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

#include <jobs/job_export_sch_plot.h>
#include <jobs/job_registry.h>
#include <i18n_utility.h>

NLOHMANN_JSON_SERIALIZE_ENUM( JOB_PAGE_SIZE,
                              {
                                      { JOB_PAGE_SIZE::PAGE_SIZE_AUTO, "auto" },
                                      { JOB_PAGE_SIZE::PAGE_SIZE_A4, "A4" },
                                      { JOB_PAGE_SIZE::PAGE_SIZE_A, "A" },
                              } )

NLOHMANN_JSON_SERIALIZE_ENUM( SCH_PLOT_FORMAT,
                              {
                                      { SCH_PLOT_FORMAT::HPGL, "hpgl" },
                                      { SCH_PLOT_FORMAT::PDF, "pdf" },
                                      { SCH_PLOT_FORMAT::POST, "post" },
                                      { SCH_PLOT_FORMAT::SVG, "svg" },
                                      { SCH_PLOT_FORMAT::DXF, "dxf" },
                              } )

JOB_EXPORT_SCH_PLOT::JOB_EXPORT_SCH_PLOT( bool aOutputIsDirectory ) :
        JOB( "plot", aOutputIsDirectory ),
        m_plotFormat( SCH_PLOT_FORMAT::PDF ),
        m_filename(),
        m_drawingSheet(),
        m_plotAll( true ),
        m_plotDrawingSheet( true ),
        m_show_hop_over( false ),
        m_blackAndWhite( false ),
        m_pageSizeSelect( JOB_PAGE_SIZE::PAGE_SIZE_AUTO ),
        m_useBackgroundColor( true ),
        m_minPenWidth( 847 /* hairline @ 300dpi */ ),
        m_PDFPropertyPopups( true ),
        m_PDFHierarchicalLinks( true ),
        m_PDFMetadata( true ),
        m_theme(),
        m_variantNames()
{
    m_params.emplace_back( new JOB_PARAM<SCH_PLOT_FORMAT>( "format",
            &m_plotFormat, m_plotFormat ) );

    m_params.emplace_back( new JOB_PARAM<wxString>( "drawing_sheet",
            &m_drawingSheet, m_drawingSheet ) );

    m_params.emplace_back( new JOB_PARAM<bool>( "plot_all",
            &m_plotAll, m_plotAll ) );

    m_params.emplace_back( new JOB_PARAM<bool>( "plot_drawing_sheet",
            &m_plotDrawingSheet, m_plotDrawingSheet ) );

    m_params.emplace_back( new JOB_PARAM<bool>( "black_and_white",
            &m_blackAndWhite, m_blackAndWhite ) );

    m_params.emplace_back( new JOB_PARAM<bool>( "show_hop_over",
            &m_show_hop_over, m_show_hop_over ) );

    m_params.emplace_back( new JOB_PARAM<JOB_PAGE_SIZE>( "page_size",
            &m_pageSizeSelect, m_pageSizeSelect ) );

    m_params.emplace_back( new JOB_PARAM<bool>( "use_background_color",
            &m_useBackgroundColor, m_useBackgroundColor ) );

    m_params.emplace_back( new JOB_PARAM<int>( "min_pen_width",
            &m_minPenWidth, m_minPenWidth ) );

    m_params.emplace_back( new JOB_PARAM<bool>( "pdf_property_popups",
            &m_PDFPropertyPopups, m_PDFPropertyPopups ) );

    m_params.emplace_back( new JOB_PARAM<bool>( "pdf_hierarchical_links",
            &m_PDFHierarchicalLinks, m_PDFHierarchicalLinks ) );

    m_params.emplace_back( new JOB_PARAM<bool>( "pdf_metadata",
            &m_PDFMetadata, m_PDFMetadata ) );

    m_params.emplace_back( new JOB_PARAM<wxString>( "color_theme",
            &m_theme, m_theme ) );

    m_params.emplace_back( new JOB_PARAM<wxString>( "variant_name", &m_variant, m_variant ) );
}


JOB_EXPORT_SCH_PLOT_PDF::JOB_EXPORT_SCH_PLOT_PDF( bool aOutputIsDirectory ) :
		JOB_EXPORT_SCH_PLOT( aOutputIsDirectory )
{
    m_plotFormat = SCH_PLOT_FORMAT::PDF;
}


wxString JOB_EXPORT_SCH_PLOT_PDF::GetDefaultDescription() const
{
    return _( "Export PDF" );
}


wxString JOB_EXPORT_SCH_PLOT_PDF::GetSettingsDialogTitle() const
{
    return _( "Export PDF Job Settings" );
}


JOB_EXPORT_SCH_PLOT_DXF ::JOB_EXPORT_SCH_PLOT_DXF () :
		JOB_EXPORT_SCH_PLOT( true )
{
	m_plotFormat = SCH_PLOT_FORMAT::DXF;
}


wxString JOB_EXPORT_SCH_PLOT_DXF::GetDefaultDescription() const
{
    return _( "Export DXF" );
}


wxString JOB_EXPORT_SCH_PLOT_DXF::GetSettingsDialogTitle() const
{
    return _( "Export DXF Job Settings" );
}


JOB_EXPORT_SCH_PLOT_SVG::JOB_EXPORT_SCH_PLOT_SVG() :
		JOB_EXPORT_SCH_PLOT( true )
{
	m_plotFormat = SCH_PLOT_FORMAT::SVG;
}


wxString JOB_EXPORT_SCH_PLOT_SVG::GetDefaultDescription() const
{
    return _( "Export SVG" );
}


wxString JOB_EXPORT_SCH_PLOT_SVG::GetSettingsDialogTitle() const
{
    return _( "Export SVG Job Settings" );
}


JOB_EXPORT_SCH_PLOT_PS::JOB_EXPORT_SCH_PLOT_PS() :
		JOB_EXPORT_SCH_PLOT( true )
{
	m_plotFormat = SCH_PLOT_FORMAT::POST;
}


wxString JOB_EXPORT_SCH_PLOT_PS::GetDefaultDescription() const
{
    return _( "Export Postscript" );
}


wxString JOB_EXPORT_SCH_PLOT_PS::GetSettingsDialogTitle() const
{
    return _( "Export Postscript Job Settings" );
}


JOB_EXPORT_SCH_PLOT_HPGL::JOB_EXPORT_SCH_PLOT_HPGL() :
		JOB_EXPORT_SCH_PLOT( true )
{
	m_plotFormat = SCH_PLOT_FORMAT::HPGL;
}


wxString JOB_EXPORT_SCH_PLOT_HPGL::GetDefaultDescription() const
{
    return _( "Export HPGL" );
}


REGISTER_JOB( sch_export_plot_svg, _HKI( "Schematic: Export SVG" ), KIWAY::FACE_SCH,
              JOB_EXPORT_SCH_PLOT_SVG );
REGISTER_DEPRECATED_JOB( sch_export_plot_hpgl, _HKI( "Schematic: Export HPGL" ), KIWAY::FACE_SCH,
              JOB_EXPORT_SCH_PLOT_HPGL );
REGISTER_JOB( sch_export_plot_ps, _HKI( "Schematic: Export Postscript" ), KIWAY::FACE_SCH,
              JOB_EXPORT_SCH_PLOT_PS );
REGISTER_JOB( sch_export_plot_dxf, _HKI( "Schematic: Export DXF" ), KIWAY::FACE_SCH,
              JOB_EXPORT_SCH_PLOT_DXF );
REGISTER_JOB( sch_export_plot_pdf, _HKI( "Schematic: Export PDF" ), KIWAY::FACE_SCH,
              JOB_EXPORT_SCH_PLOT_PDF );