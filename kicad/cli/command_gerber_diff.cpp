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

#include "command_gerber_diff.h"
#include <cli/exit_codes.h>
#include <jobs/job_gerber_diff.h>
#include <kiface_base.h>
#include <string_utils.h>
#include <macros.h>
#include <wx/crt.h>
#include <wx/filename.h>


#define ARG_FILE_A "file1"
#define ARG_FILE_B "file2"
#define ARG_FORMAT "--format"
#define ARG_DPI "--dpi"
#define ARG_NO_ANTIALIAS "--no-antialias"
#define ARG_TRANSPARENT "--transparent"
#define ARG_EXIT_CODE_ONLY "--exit-code-only"
#define ARG_TOLERANCE "--tolerance"
#define ARG_STRICT "--strict"
#define ARG_NO_ALIGN "--no-align"


CLI::GERBER_DIFF_COMMAND::GERBER_DIFF_COMMAND() :
        COMMAND( "diff" )
{
    addCommonArgs( false, true, IO_TYPE::FILE, IO_TYPE::FILE );

    m_argParser.add_description( UTF8STDSTR( _( "Compare two Gerber or Excellon files and show differences" ) ) );

    m_argParser.add_argument( ARG_FILE_A )
            .help( UTF8STDSTR( _( "Reference file (first Gerber/Excellon file)" ) ) )
            .metavar( "FILE1" );

    m_argParser.add_argument( ARG_FILE_B )
            .help( UTF8STDSTR( _( "Comparison file (second Gerber/Excellon file)" ) ) )
            .metavar( "FILE2" );

    m_argParser.add_argument( ARG_FORMAT )
            .default_value( std::string( "png" ) )
            .help( UTF8STDSTR( _( "Output format: png, text, or json (default: png)" ) ) )
            .metavar( "FORMAT" );

    m_argParser.add_argument( ARG_DPI )
            .default_value( 300 )
            .scan<'i', int>()
            .help( UTF8STDSTR( _( "Resolution in DPI for PNG output (default: 300)" ) ) )
            .metavar( "DPI" );

    m_argParser.add_argument( ARG_NO_ANTIALIAS )
            .help( UTF8STDSTR( _( "Disable anti-aliasing for PNG output" ) ) )
            .flag();

    m_argParser.add_argument( ARG_TRANSPARENT )
            .help( UTF8STDSTR( _( "Use transparent background for PNG output" ) ) )
            .flag();

    m_argParser.add_argument( ARG_EXIT_CODE_ONLY )
            .help( UTF8STDSTR( _( "Only set exit code (0=identical, 1=different), no output" ) ) )
            .flag();

    m_argParser.add_argument( ARG_TOLERANCE )
            .default_value( 0 )
            .scan<'i', int>()
            .help( UTF8STDSTR( _( "Tolerance in nm for floating point comparisons (default: 0)" ) ) )
            .metavar( "TOLERANCE" );

    m_argParser.add_argument( ARG_STRICT ).help( UTF8STDSTR( _( "Fail on any parse warnings or errors" ) ) ).flag();

    m_argParser.add_argument( ARG_NO_ALIGN )
            .help( UTF8STDSTR( _( "Skip bounding-box origin alignment; catches absolute-placement regressions" ) ) )
            .flag();
}


int CLI::GERBER_DIFF_COMMAND::doPerform( KIWAY& aKiway )
{
    std::unique_ptr<JOB_GERBER_DIFF> diffJob = std::make_unique<JOB_GERBER_DIFF>();

    diffJob->m_inputFileA = From_UTF8( m_argParser.get<std::string>( ARG_FILE_A ).c_str() );
    diffJob->m_inputFileB = From_UTF8( m_argParser.get<std::string>( ARG_FILE_B ).c_str() );
    diffJob->m_dpi = m_argParser.get<int>( ARG_DPI );
    diffJob->m_antialias = !m_argParser.get<bool>( ARG_NO_ANTIALIAS );
    diffJob->m_transparentBackground = m_argParser.get<bool>( ARG_TRANSPARENT );
    diffJob->m_exitCodeOnly = m_argParser.get<bool>( ARG_EXIT_CODE_ONLY );
    diffJob->m_tolerance = m_argParser.get<int>( ARG_TOLERANCE );
    diffJob->m_strict = m_argParser.get<bool>( ARG_STRICT );
    diffJob->m_noAlign = m_argParser.get<bool>( ARG_NO_ALIGN );

    wxString format = From_UTF8( m_argParser.get<std::string>( ARG_FORMAT ).c_str() );

    if( format == wxS( "png" ) )
    {
        diffJob->m_outputFormat = JOB_GERBER_DIFF::OUTPUT_FORMAT::PNG;
    }
    else if( format == wxS( "text" ) )
    {
        diffJob->m_outputFormat = JOB_GERBER_DIFF::OUTPUT_FORMAT::TEXT;
    }
    else if( format == wxS( "json" ) )
    {
        diffJob->m_outputFormat = JOB_GERBER_DIFF::OUTPUT_FORMAT::JSON;
    }
    else
    {
        wxFprintf( stderr, _( "Invalid output format: %s\n" ), format );
        return EXIT_CODES::ERR_ARGS;
    }

    // For PNG, generate a default output path when -o is not given.
    // For text/JSON, -o is optional — omitting it sends output to stdout.
    if( diffJob->m_outputFormat == JOB_GERBER_DIFF::OUTPUT_FORMAT::PNG )
    {
        if( m_argOutput.IsEmpty() )
        {
            wxFileName inputFn( diffJob->m_inputFileA );
            diffJob->SetConfiguredOutputPath( inputFn.GetPath() + wxFileName::GetPathSeparator() + inputFn.GetName()
                                              + wxS( "-diff.png" ) );
        }
        else
        {
            diffJob->SetConfiguredOutputPath( m_argOutput );
        }
    }
    else if( !m_argOutput.IsEmpty() )
    {
        diffJob->SetConfiguredOutputPath( m_argOutput );
    }

    int exitCode = aKiway.ProcessJob( KIWAY::FACE_GERBVIEW, diffJob.get() );

    return exitCode;
}
