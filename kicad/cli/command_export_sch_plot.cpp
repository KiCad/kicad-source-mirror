/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 1992-2022 KiCad Developers, see AUTHORS.txt for contributors.
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
#include "command_export_sch_plot.h"
#include <cli/exit_codes.h>
#include "jobs/job_export_sch_plot.h"
#include <layer_ids.h>
#include <wx/crt.h>

#include <macros.h>
#include <wx/tokenzr.h>

#define ARG_EXCLUDE_DRAWING_SHEET "--exclude-drawing-sheet"
#define ARG_NO_BACKGROUND_COLOR "--no-background-color"
#define ARG_PLOT_ONE "--plot-one"
#define ARG_HPGL_PEN_SIZE "--pen-size"
#define ARG_HPGL_ORIGIN "--origin"

const HPGL_PLOT_ORIGIN_AND_UNITS hpgl_origin_ops[4] = {
    HPGL_PLOT_ORIGIN_AND_UNITS::PLOTTER_BOT_LEFT, HPGL_PLOT_ORIGIN_AND_UNITS::PLOTTER_CENTER,
    HPGL_PLOT_ORIGIN_AND_UNITS::USER_FIT_PAGE, HPGL_PLOT_ORIGIN_AND_UNITS::USER_FIT_CONTENT
};

CLI::EXPORT_SCH_PLOT_COMMAND::EXPORT_SCH_PLOT_COMMAND( const std::string& aName,
                                                       PLOT_FORMAT        aPlotFormat,
                                                       bool               aOutputIsDir ) :
        EXPORT_PCB_BASE_COMMAND( aName, aOutputIsDir ),
        m_plotFormat( aPlotFormat ), m_useDir( aOutputIsDir )
{
    m_argParser.add_argument( "-t", ARG_THEME )
            .default_value( std::string() )
            .help( UTF8STDSTR( _( "Color theme to use (will default to schematic settings)" ) ) );

    m_argParser.add_argument( "-b", ARG_BLACKANDWHITE )
            .help( UTF8STDSTR( _( ARG_BLACKANDWHITE_DESC ) ) )
            .implicit_value( true )
            .default_value( false );

    m_argParser.add_argument( "-e", ARG_EXCLUDE_DRAWING_SHEET )
            .help( UTF8STDSTR( _( "No drawing sheet" ) ) )
            .implicit_value( true )
            .default_value( false );

    m_argParser.add_argument( "-n", ARG_NO_BACKGROUND_COLOR )
            .help( UTF8STDSTR( _( "Avoid setting a background color (regardless of theme)" ) ) )
            .implicit_value( true )
            .default_value( false );

    m_argParser.add_argument( "-O", ARG_PLOT_ONE )
            .help( UTF8STDSTR( _( "Plot only the first page (no sub-sheets)" ) ) )
            .implicit_value( true )
            .default_value( false );

    if( aPlotFormat == PLOT_FORMAT::HPGL )
    {
        m_argParser.add_argument( "-p", ARG_HPGL_PEN_SIZE )
                .help( UTF8STDSTR( _( "Pen size [mm]" ) ) )
                .scan<'g', double>()
                .default_value( 0.5 );

        m_argParser.add_argument( "-r", ARG_HPGL_ORIGIN )
                .help( UTF8STDSTR( _( "Origin and scale: 0 bottom left, 1 centered, 2 page fit, 3 "
                                      "content fit" ) ) )
                .scan<'d', int>()
                .default_value( 1 );
    }
}


int CLI::EXPORT_SCH_PLOT_COMMAND::doPerform( KIWAY& aKiway )
{
    wxString filename = FROM_UTF8( m_argParser.get<std::string>( ARG_INPUT ).c_str() );
    if( !wxFile::Exists( filename ) )
    {
        wxFprintf( stderr, _( "Schematic file does not exist or is not accessible\n" ) );
        return EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    std::unique_ptr<JOB_EXPORT_SCH_PLOT> plotJob =
            std::make_unique<JOB_EXPORT_SCH_PLOT>( true, m_plotFormat, filename );

    SCH_PLOT_SETTINGS& settings = plotJob->settings;
    settings.m_plotAll = !m_argParser.get<bool>( ARG_PLOT_ONE );
    settings.m_plotDrawingSheet = !m_argParser.get<bool>( ARG_EXCLUDE_DRAWING_SHEET );
    settings.m_blackAndWhite = m_argParser.get<bool>( ARG_BLACKANDWHITE );
    settings.m_pageSizeSelect = PAGE_SIZE_AUTO;
    settings.m_useBackgroundColor = !m_argParser.get<bool>( ARG_NO_BACKGROUND_COLOR );
    settings.m_theme = FROM_UTF8( m_argParser.get<std::string>( ARG_THEME ).c_str() );
    if( m_useDir )
        settings.m_outputDirectory =
                FROM_UTF8( m_argParser.get<std::string>( ARG_OUTPUT ).c_str() );
    else
        settings.m_outputFile = FROM_UTF8( m_argParser.get<std::string>( ARG_OUTPUT ).c_str() );
    // HPGL local options
    if( m_plotFormat == PLOT_FORMAT::HPGL )
    {
        settings.m_HPGLPenSize =
                m_argParser.get<double>( ARG_HPGL_PEN_SIZE ) * schIUScale.IU_PER_MM;
        int origin = m_argParser.get<int>( ARG_HPGL_ORIGIN );
        if( origin < 0 || origin > 3 )
        {
            wxFprintf( stderr, _( "HPGL origin option must be 0, 1, 2 or 3\n" ) );
            return EXIT_CODES::ERR_ARGS;
        }
        settings.m_HPGLPlotOrigin = hpgl_origin_ops[origin];
    }

    int exitCode = aKiway.ProcessJob( KIWAY::FACE_SCH, plotJob.get() );

    return exitCode;
}