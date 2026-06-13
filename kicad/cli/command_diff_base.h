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
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef KICAD_CLI_COMMAND_DIFF_BASE_H
#define KICAD_CLI_COMMAND_DIFF_BASE_H

#include "command.h"

#include <cli/diff_format_helpers.h>
#include <cli/exit_codes.h>

#include <kiway.h>
#include <string_utils.h>

#include <memory>
#include <string>

#include <wx/crt.h>


namespace CLI
{

/**
 * Shared `diff` subcommand for the document/library differs.
 *
 * Registers the input pair plus the common --format / --output /
 * --exit-code-only options, then dispatches a JOB_T (a JOB_DIFF_BASE subclass)
 * to @p aFace. Subclasses supply only the localized strings, the positional
 * argument names/metavars (file pair vs directory pair), and the target kiface.
 */
template <typename JOB_T>
class DIFF_COMMAND : public COMMAND
{
public:
    DIFF_COMMAND( const wxString& aDescription, const std::string& aArgA, const wxString& aHelpA,
                  const std::string& aMetaA, const std::string& aArgB, const wxString& aHelpB,
                  const std::string& aMetaB, KIWAY::FACE_T aFace ) :
            COMMAND( "diff" ),
            m_argA( aArgA ),
            m_argB( aArgB ),
            m_face( aFace )
    {
        m_argParser.add_description( UTF8STDSTR( aDescription ) );

        m_argParser.add_argument( m_argA ).help( UTF8STDSTR( aHelpA ) ).metavar( aMetaA );
        m_argParser.add_argument( m_argB ).help( UTF8STDSTR( aHelpB ) ).metavar( aMetaB );

        m_argParser.add_argument( "--format" )
                .default_value( std::string( "json" ) )
                .help( UTF8STDSTR( _( "Output format: json, text, png, or svg (default: json)" ) ) )
                .metavar( "FORMAT" );

        m_argParser.add_argument( "-o", "--output" )
                .default_value( std::string( "" ) )
                .help( UTF8STDSTR( _( "Output file path; required for png/svg, otherwise prints to "
                                      "stdout if omitted" ) ) )
                .metavar( "PATH" );

        m_argParser.add_argument( "--exit-code-only" )
                .help( UTF8STDSTR( _( "Only set exit code (0=identical, 5=differences found, "
                                      "non-zero on error), no output" ) ) )
                .flag();
    }

protected:
    int doPerform( KIWAY& aKiway ) override
    {
        std::unique_ptr<JOB_T> job = std::make_unique<JOB_T>();

        job->m_inputA       = From_UTF8( m_argParser.get<std::string>( m_argA ).c_str() );
        job->m_inputB       = From_UTF8( m_argParser.get<std::string>( m_argB ).c_str() );
        job->m_exitCodeOnly = m_argParser.get<bool>( "--exit-code-only" );

        job->SetConfiguredOutputPath( From_UTF8( m_argParser.get<std::string>( "--output" ).c_str() ) );

        std::string fmt = m_argParser.get<std::string>( "--format" );

        if( !ParseDiffOutputFormat( fmt, job->m_format ) )
        {
            ReportInvalidDiffOutputFormat( fmt );
            return EXIT_CODES::ERR_ARGS;
        }

        if( DiffOutputFormatRequiresOutputPath( job->m_format )
            && job->GetConfiguredOutputPath().IsEmpty() )
        {
            wxFprintf( stderr, _( "--output is required for png/svg formats\n" ) );
            return EXIT_CODES::ERR_ARGS;
        }

        return aKiway.ProcessJob( m_face, job.get() );
    }

private:
    std::string   m_argA;
    std::string   m_argB;
    KIWAY::FACE_T m_face;
};

} // namespace CLI

#endif // KICAD_CLI_COMMAND_DIFF_BASE_H
