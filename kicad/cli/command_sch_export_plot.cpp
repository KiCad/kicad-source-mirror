/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include "jobs/job_export_sch_plot.h"
#include <layer_ids.h>
#include <wx/crt.h>
#include <string_utils.h>

#include <macros.h>
#include <wx/tokenzr.h>

#define ARG_EXCLUDE_DRAWING_SHEET "--exclude-drawing-sheet"
#define ARG_NO_BACKGROUND_COLOR "--no-background-color"
#define ARG_HPGL_PEN_SIZE "--pen-size"
#define ARG_HPGL_ORIGIN "--origin"
#define ARG_PAGES "--pages"
#define ARG_EXCLUDE_PDF_PROPERTY_POPUPS "--exclude-pdf-property-popups"
#define ARG_EXCLUDE_PDF_METADATA "--exclude-pdf-metadata"

const JOB_HPGL_PLOT_ORIGIN_AND_UNITS hpgl_origin_ops[4] = {
    JOB_HPGL_PLOT_ORIGIN_AND_UNITS::PLOTTER_BOT_LEFT,
    JOB_HPGL_PLOT_ORIGIN_AND_UNITS::PLOTTER_CENTER,
    JOB_HPGL_PLOT_ORIGIN_AND_UNITS::USER_FIT_PAGE,
    JOB_HPGL_PLOT_ORIGIN_AND_UNITS::USER_FIT_CONTENT
};

CLI::SCH_EXPORT_PLOT_COMMAND::SCH_EXPORT_PLOT_COMMAND( const std::string& aName,
                                                       const std::string& aDescription,
                                                       SCH_PLOT_FORMAT        aPlotFormat,
                                                       bool               aOutputIsDir ) :
        COMMAND( aName ),
        m_plotFormat( aPlotFormat )
{
    m_argParser.add_description( aDescription );

    addCommonArgs( true, true, false, aOutputIsDir );
    addDrawingSheetArg();
    addDefineArg();

    if( aPlotFormat != SCH_PLOT_FORMAT::HPGL )
    {
        m_argParser.add_argument( "-t", ARG_THEME )
                .default_value( std::string() )
                .help( UTF8STDSTR( _( "Color theme to use (will default to schematic "
                                      "settings)" ) ) )
                .metavar( "THEME_NAME" );
        m_argParser.add_argument( "-b", ARG_BLACKANDWHITE )
                .help( UTF8STDSTR( _( ARG_BLACKANDWHITE_DESC ) ) )
                .flag();
    }

    m_argParser.add_argument( "-e", ARG_EXCLUDE_DRAWING_SHEET )
            .help( UTF8STDSTR( _( "No drawing sheet" ) ) )
            .implicit_value( true )
            .default_value( false );

    if( aPlotFormat == SCH_PLOT_FORMAT::PDF )
    {
        m_argParser.add_argument( ARG_EXCLUDE_PDF_PROPERTY_POPUPS )
                .help( UTF8STDSTR( _( "Do not generate property popups in PDF" ) ) )
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

    m_argParser.add_argument( "-p", ARG_PAGES )
            .default_value( std::string() )
            .help( UTF8STDSTR( _( "List of page numbers separated by comma to print, blank or "
                                  "unspecified is equivalent to all pages" ) ) )
            .metavar( "PAGE_LIST" );

    if( aPlotFormat == SCH_PLOT_FORMAT::HPGL )
    {
        m_argParser.add_argument( "-p", ARG_HPGL_PEN_SIZE )
                .help( UTF8STDSTR( _( "Pen size [mm]" ) ) )
                .scan<'g', double>()
                .default_value( 0.5 )
                .metavar( "PEN_SIZE" );

        m_argParser.add_argument( "-r", ARG_HPGL_ORIGIN )
                .help( UTF8STDSTR( _( "Origin and scale: 0 bottom left, 1 centered, 2 page fit, 3 "
                                      "content fit" ) ) )
                .scan<'d', int>()
                .default_value( 1 )
                .metavar( "ORIGIN" );
    }
}


int CLI::SCH_EXPORT_PLOT_COMMAND::doPerform( KIWAY& aKiway )
{
    wxString filename = m_argInput;
    if( !wxFile::Exists( filename ) )
    {
        wxFprintf( stderr, _( "Schematic file does not exist or is not accessible\n" ) );
        return EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    std::vector<wxString> pages;
    wxString              pagesStr = From_UTF8( m_argParser.get<std::string>( ARG_PAGES ).c_str() );
    wxStringTokenizer     tokenizer( pagesStr, "," );
    while( tokenizer.HasMoreTokens() )
    {
        pages.push_back( tokenizer.GetNextToken().Trim() );
    }

    std::unique_ptr<JOB_EXPORT_SCH_PLOT> plotJob =
            std::make_unique<JOB_EXPORT_SCH_PLOT>( true, m_plotFormat, filename );

    plotJob->m_plotPages = pages;
    plotJob->m_plotDrawingSheet = !m_argParser.get<bool>( ARG_EXCLUDE_DRAWING_SHEET );
    plotJob->m_pageSizeSelect = JOB_PAGE_SIZE::PAGE_SIZE_AUTO;

    if( m_plotFormat == SCH_PLOT_FORMAT::PDF
            || m_plotFormat == SCH_PLOT_FORMAT::POST
            || m_plotFormat == SCH_PLOT_FORMAT::SVG )
    {
        plotJob->m_useBackgroundColor = !m_argParser.get<bool>( ARG_NO_BACKGROUND_COLOR );
    }

    if ( m_plotFormat != SCH_PLOT_FORMAT::HPGL )
    {
        plotJob->m_blackAndWhite = m_argParser.get<bool>( ARG_BLACKANDWHITE );
        plotJob->m_theme = From_UTF8( m_argParser.get<std::string>( ARG_THEME ).c_str() );
    }

    if( m_outputArgExpectsDir )
        plotJob->m_outputDirectory = m_argOutput;
    else
        plotJob->m_outputFile = m_argOutput;

    plotJob->m_plotAll = plotJob->m_plotPages.size() == 0;

    plotJob->m_drawingSheet = m_argDrawingSheet;
    plotJob->SetVarOverrides( m_argDefineVars );

    // HPGL local options
    if( m_plotFormat == SCH_PLOT_FORMAT::HPGL )
    {
        plotJob->m_HPGLPenSize =
                m_argParser.get<double>( ARG_HPGL_PEN_SIZE ) * schIUScale.IU_PER_MM;
        int origin = m_argParser.get<int>( ARG_HPGL_ORIGIN );
        if( origin < 0 || origin > 3 )
        {
            wxFprintf( stderr, _( "HPGL origin option must be 0, 1, 2 or 3\n" ) );
            return EXIT_CODES::ERR_ARGS;
        }
        plotJob->m_HPGLPlotOrigin = hpgl_origin_ops[origin];
    }
    // PDF local options
    else if( m_plotFormat == SCH_PLOT_FORMAT::PDF )
    {
        plotJob->m_PDFPropertyPopups = !m_argParser.get<bool>( ARG_EXCLUDE_PDF_PROPERTY_POPUPS );
        plotJob->m_PDFMetadata = !m_argParser.get<bool>( ARG_EXCLUDE_PDF_METADATA );
    }

    int exitCode = aKiway.ProcessJob( KIWAY::FACE_SCH, plotJob.get() );

    return exitCode;
}
