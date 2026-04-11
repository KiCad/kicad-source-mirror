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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "command_gerber_convert_png.h"
#include <cli/exit_codes.h>
#include <jobs/job_gerber_export_png.h>
#include <kiface_base.h>
#include <string_utils.h>
#include <macros.h>
#include <wx/crt.h>
#include <wx/filename.h>


#define ARG_DPI "--dpi"
#define ARG_WIDTH "--width"
#define ARG_HEIGHT "--height"
#define ARG_NO_ANTIALIAS "--no-antialias"
#define ARG_TRANSPARENT "--transparent"
#define ARG_STRICT "--strict"
#define ARG_UNITS "--units"
#define ARG_ORIGIN_X "--origin-x"
#define ARG_ORIGIN_Y "--origin-y"
#define ARG_WINDOW_WIDTH "--window-width"
#define ARG_WINDOW_HEIGHT "--window-height"
#define ARG_FOREGROUND "--foreground"
#define ARG_BACKGROUND "--background"


CLI::GERBER_CONVERT_PNG_COMMAND::GERBER_CONVERT_PNG_COMMAND() :
        COMMAND( "png" )
{
    addCommonArgs( true, true, IO_TYPE::FILE, IO_TYPE::FILE );

    m_argParser.add_description( UTF8STDSTR( _( "Convert a Gerber or Excellon file to PNG image" ) ) );

    m_argParser.add_argument( ARG_DPI )
            .default_value( 300 )
            .scan<'i', int>()
            .help( UTF8STDSTR( _( "Resolution in DPI (default: 300)" ) ) )
            .metavar( "DPI" );

    m_argParser.add_argument( ARG_WIDTH )
            .default_value( 0 )
            .scan<'i', int>()
            .help( UTF8STDSTR( _( "Output width in pixels (overrides DPI)" ) ) )
            .metavar( "PIXELS" );

    m_argParser.add_argument( ARG_HEIGHT )
            .default_value( 0 )
            .scan<'i', int>()
            .help( UTF8STDSTR( _( "Output height in pixels (overrides DPI)" ) ) )
            .metavar( "PIXELS" );

    m_argParser.add_argument( ARG_NO_ANTIALIAS ).help( UTF8STDSTR( _( "Disable anti-aliasing" ) ) ).flag();

    m_argParser.add_argument( ARG_TRANSPARENT )
            .help( UTF8STDSTR( _( "Use transparent background (default)" ) ) )
            .flag();

    m_argParser.add_argument( ARG_STRICT ).help( UTF8STDSTR( _( "Fail on any parse warnings or errors" ) ) ).flag();

    m_argParser.add_argument( ARG_UNITS )
            .default_value( std::string( "mm" ) )
            .help( UTF8STDSTR( _( "Units for viewport parameters, options: mm, inch, mils (default: mm)" ) ) )
            .metavar( "UNITS" );

    m_argParser.add_argument( ARG_ORIGIN_X )
            .default_value( 0.0 )
            .scan<'g', double>()
            .help( UTF8STDSTR( _( "Viewport origin X" ) ) )
            .metavar( "VALUE" );

    m_argParser.add_argument( ARG_ORIGIN_Y )
            .default_value( 0.0 )
            .scan<'g', double>()
            .help( UTF8STDSTR( _( "Viewport origin Y" ) ) )
            .metavar( "VALUE" );

    m_argParser.add_argument( ARG_WINDOW_WIDTH )
            .default_value( 0.0 )
            .scan<'g', double>()
            .help( UTF8STDSTR( _( "Viewport width (enables viewport mode)" ) ) )
            .metavar( "VALUE" );

    m_argParser.add_argument( ARG_WINDOW_HEIGHT )
            .default_value( 0.0 )
            .scan<'g', double>()
            .help( UTF8STDSTR( _( "Viewport height (enables viewport mode)" ) ) )
            .metavar( "VALUE" );

    m_argParser.add_argument( ARG_FOREGROUND )
            .default_value( std::string() )
            .help( UTF8STDSTR( _( "Foreground color as hex (e.g., '#FFFFFF')" ) ) )
            .metavar( "COLOR" );

    m_argParser.add_argument( ARG_BACKGROUND )
            .default_value( std::string() )
            .help( UTF8STDSTR( _( "Background color as hex (e.g., '#000000')" ) ) )
            .metavar( "COLOR" );
}


int CLI::GERBER_CONVERT_PNG_COMMAND::doPerform( KIWAY& aKiway )
{
    std::unique_ptr<JOB_GERBER_EXPORT_PNG> pngJob = std::make_unique<JOB_GERBER_EXPORT_PNG>();

    pngJob->m_inputFile = m_argInput;
    pngJob->m_dpi = m_argParser.get<int>( ARG_DPI );
    pngJob->m_width = m_argParser.get<int>( ARG_WIDTH );
    pngJob->m_height = m_argParser.get<int>( ARG_HEIGHT );
    pngJob->m_antialias = !m_argParser.get<bool>( ARG_NO_ANTIALIAS );
    pngJob->m_transparentBackground = m_argParser.get<bool>( ARG_TRANSPARENT );
    pngJob->m_strict = m_argParser.get<bool>( ARG_STRICT );

    wxString units = From_UTF8( m_argParser.get<std::string>( ARG_UNITS ).c_str() );

    if( units == wxS( "mm" ) )
    {
        pngJob->m_units = JOB_GERBER_EXPORT_PNG::UNITS::MM;
    }
    else if( units == wxS( "inch" ) )
    {
        pngJob->m_units = JOB_GERBER_EXPORT_PNG::UNITS::INCH;
    }
    else if( units == wxS( "mils" ) )
    {
        pngJob->m_units = JOB_GERBER_EXPORT_PNG::UNITS::MILS;
    }
    else
    {
        wxFprintf( stderr, _( "Invalid units: %s\n" ), units );
        return CLI::EXIT_CODES::ERR_ARGS;
    }

    pngJob->m_originX = m_argParser.get<double>( ARG_ORIGIN_X );
    pngJob->m_originY = m_argParser.get<double>( ARG_ORIGIN_Y );
    pngJob->m_windowWidth = m_argParser.get<double>( ARG_WINDOW_WIDTH );
    pngJob->m_windowHeight = m_argParser.get<double>( ARG_WINDOW_HEIGHT );

    if( ( pngJob->m_windowWidth > 0.0 ) != ( pngJob->m_windowHeight > 0.0 ) )
    {
        wxFprintf( stderr, _( "Error: both --window-width and --window-height must be specified together\n" ) );
        return CLI::EXIT_CODES::ERR_ARGS;
    }

    pngJob->m_foregroundColor = wxString::FromUTF8( m_argParser.get<std::string>( ARG_FOREGROUND ) );
    pngJob->m_backgroundColor = wxString::FromUTF8( m_argParser.get<std::string>( ARG_BACKGROUND ) );

    // Handle output path
    if( m_argOutput.IsEmpty() )
    {
        wxFileName inputFn( m_argInput );
        pngJob->SetConfiguredOutputPath( inputFn.GetPath() + wxFileName::GetPathSeparator() + inputFn.GetName()
                                         + wxS( ".png" ) );
    }
    else
    {
        pngJob->SetConfiguredOutputPath( m_argOutput );
    }

    int exitCode = aKiway.ProcessJob( KIWAY::FACE_GERBVIEW, pngJob.get() );

    return exitCode;
}
