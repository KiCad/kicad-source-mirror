/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
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

#include <kiface_base.h>
#include <sch_plotter.h>
#include "command_sch_export_plot.h"
#include <cli/exit_codes.h>
#include "font/kicad_font_name.h"
#include "jobs/job_export_sch_plot.h"
#include <layer_ids.h>
#include <wx/crt.h>
#include <string_utils.h>

#include <macros.h>
#include <wx/tokenzr.h>

#define ARG_EXCLUDE_DRAWING_SHEET "--exclude-drawing-sheet"
#define ARG_NO_BACKGROUND_COLOR "--no-background-color"
#define ARG_PAGES "--pages"
#define ARG_EXCLUDE_PDF_PROPERTY_POPUPS "--exclude-pdf-property-popups"
#define ARG_EXCLUDE_PDF_HIERARCHICAL_LINKS "--exclude-pdf-hierarchical-links"
#define ARG_EXCLUDE_PDF_METADATA "--exclude-pdf-metadata"
#define ARG_FONT_NAME "--default-font"
#define ARG_DRAW_HOP_OVER "--draw-hop-over"

#define DEPRECATED_ARG_HPGL_PEN_SIZE "--pen-size"
#define DEPRECATED_ARG_HPGL_ORIGIN "--origin"

CLI::SCH_EXPORT_PLOT_COMMAND::SCH_EXPORT_PLOT_COMMAND( const std::string& aName,
                                                       const std::string& aDescription,
                                                       SCH_PLOT_FORMAT    aPlotFormat,
                                                       bool               aOutputIsDir ) :
        COMMAND( aName ),
        m_plotFormat( aPlotFormat )
{
    m_argParser.add_description( aDescription );

    addCommonArgs( true, true, false, aOutputIsDir );
    addDrawingSheetArg();
    addDefineArg();
    addVariantsArg();

    m_argParser.add_argument( "-t", ARG_THEME )
            .default_value( std::string() )
            .help( UTF8STDSTR( _( "Color theme to use (will default to schematic settings)" ) ) )
            .metavar( "THEME_NAME" );
    m_argParser.add_argument( "-b", ARG_BLACKANDWHITE )
            .help( UTF8STDSTR( _( ARG_BLACKANDWHITE_DESC ) ) )
            .flag();

    m_argParser.add_argument( "-e", ARG_EXCLUDE_DRAWING_SHEET )
            .help( UTF8STDSTR( _( "No drawing sheet" ) ) )
            .flag();

    m_argParser.add_argument( ARG_FONT_NAME )
            .help( UTF8STDSTR( _( "Default font name" ) ) )
            .default_value( wxString( "" ).ToStdString() );

    m_argParser.add_argument( ARG_DRAW_HOP_OVER )
            .help( UTF8STDSTR( _( "Draw hop over at wire crossings" ) ) )
            .flag();

    if( aPlotFormat == SCH_PLOT_FORMAT::PDF )
    {
        m_argParser.add_argument( ARG_EXCLUDE_PDF_PROPERTY_POPUPS )
                .help( UTF8STDSTR( _( "Do not generate property popups in PDF" ) ) )
                .flag();

        m_argParser.add_argument( ARG_EXCLUDE_PDF_HIERARCHICAL_LINKS )
                .help( UTF8STDSTR( _( "Do not generate clickable links for hierarchical elements in PDF" ) ) )
                .flag();

        m_argParser.add_argument( ARG_EXCLUDE_PDF_METADATA )
                .help( UTF8STDSTR( _( "Do not generate PDF metadata from AUTHOR and SUBJECT variables" ) ) )
                .flag();
    }

    if( aPlotFormat == SCH_PLOT_FORMAT::PDF
            || aPlotFormat == SCH_PLOT_FORMAT::POST
            || aPlotFormat == SCH_PLOT_FORMAT::SVG )
    {
        m_argParser.add_argument( "-n", ARG_NO_BACKGROUND_COLOR )
                .help( UTF8STDSTR( _( "Avoid setting a background color (regardless of theme)" ) ) )
                .flag();
    }

    m_argParser.add_argument( ARG_PAGES )
            .default_value( std::string() )
            .help( UTF8STDSTR( _( "List of page numbers separated by comma to print, blank or "
                                  "unspecified is equivalent to all pages" ) ) )
            .metavar( "PAGE_LIST" );

    if( aPlotFormat == SCH_PLOT_FORMAT::HPGL )
    {
        m_argParser.add_argument( "-p", DEPRECATED_ARG_HPGL_PEN_SIZE )
                .help( UTF8STDSTR( _( "Deprecated.  Has no effect." ) ) )
                .scan<'g', double>()
                .default_value( 0.5 )
                .metavar( "PEN_SIZE" );

        m_argParser.add_argument( "-r", DEPRECATED_ARG_HPGL_ORIGIN )
                .help( UTF8STDSTR( _( "Deprecated.  Has no effect." ) ) )
                .scan<'d', int>()
                .default_value( 1 )
                .metavar( "ORIGIN" );
    }
}


int CLI::SCH_EXPORT_PLOT_COMMAND::doPerform( KIWAY& aKiway )
{
    if( m_plotFormat == SCH_PLOT_FORMAT::HPGL )
    {
        wxFprintf( stderr, _( "Plotting to HPGL is no longer supported as of KiCad 10.0.\n" ) );
        return EXIT_CODES::ERR_ARGS;
    }

    wxString filename = m_argInput;

    if( !wxFile::Exists( filename ) )
    {
        wxFprintf( stderr, _( "Schematic file does not exist or is not accessible\n" ) );
        return EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    std::vector<wxString> pages;
    wxString              pagesStr = From_UTF8( m_argParser.get<std::string>( ARG_PAGES ).c_str() );
    wxStringTokenizer     tokenizer( pagesStr, ",", wxTOKEN_STRTOK );

    while( tokenizer.HasMoreTokens() )
        pages.push_back( tokenizer.GetNextToken().Trim( true ).Trim( false ) );

    std::unique_ptr<JOB_EXPORT_SCH_PLOT> plotJob;

    switch( m_plotFormat )
    {
    case SCH_PLOT_FORMAT::PDF:  plotJob = std::make_unique<JOB_EXPORT_SCH_PLOT_PDF>( false ); break;
    case SCH_PLOT_FORMAT::DXF:  plotJob = std::make_unique<JOB_EXPORT_SCH_PLOT_DXF>();        break;
    case SCH_PLOT_FORMAT::SVG:  plotJob = std::make_unique<JOB_EXPORT_SCH_PLOT_SVG>();        break;
    case SCH_PLOT_FORMAT::POST: plotJob = std::make_unique<JOB_EXPORT_SCH_PLOT_PS>();         break;
    case SCH_PLOT_FORMAT::HPGL: /* no longer supported */                                     break;
    }

    plotJob->m_filename = filename;
    plotJob->m_plotFormat = m_plotFormat;
    plotJob->m_plotPages = pages;
    plotJob->m_plotDrawingSheet = !m_argParser.get<bool>( ARG_EXCLUDE_DRAWING_SHEET );
    plotJob->m_pageSizeSelect = JOB_PAGE_SIZE::PAGE_SIZE_AUTO;
    plotJob->m_defaultFont = From_UTF8( m_argParser.get<std::string>( ARG_FONT_NAME ).c_str() );
    plotJob->m_show_hop_over = m_argParser.get<bool>( ARG_DRAW_HOP_OVER );

    if( m_plotFormat == SCH_PLOT_FORMAT::PDF
            || m_plotFormat == SCH_PLOT_FORMAT::POST
            || m_plotFormat == SCH_PLOT_FORMAT::SVG )
    {
        plotJob->m_useBackgroundColor = !m_argParser.get<bool>( ARG_NO_BACKGROUND_COLOR );
    }

    plotJob->m_blackAndWhite = m_argParser.get<bool>( ARG_BLACKANDWHITE );
    plotJob->m_theme = From_UTF8( m_argParser.get<std::string>( ARG_THEME ).c_str() );

    plotJob->SetConfiguredOutputPath( m_argOutput );

    plotJob->m_plotAll = plotJob->m_plotPages.size() == 0;

    plotJob->m_drawingSheet = m_argDrawingSheet;
    plotJob->SetVarOverrides( m_argDefineVars );
    plotJob->m_variantNames = m_argVariantNames;

    // PDF local options
    if( m_plotFormat == SCH_PLOT_FORMAT::PDF )
    {
        plotJob->m_PDFPropertyPopups = !m_argParser.get<bool>( ARG_EXCLUDE_PDF_PROPERTY_POPUPS );
        plotJob->m_PDFHierarchicalLinks = !m_argParser.get<bool>( ARG_EXCLUDE_PDF_HIERARCHICAL_LINKS );
        plotJob->m_PDFMetadata = !m_argParser.get<bool>( ARG_EXCLUDE_PDF_METADATA );
    }

    int exitCode = aKiway.ProcessJob( KIWAY::FACE_SCH, plotJob.get() );

    return exitCode;
}
