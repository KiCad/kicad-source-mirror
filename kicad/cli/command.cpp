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

#include "command.h"
#include <cli/exit_codes.h>
#include <wx/crt.h>
#include <macros.h>
#include <string_utils.h>

#include <sstream>


CLI::COMMAND::COMMAND( const std::string& aName ) :
        m_name( aName ),
        m_argParser( aName, "", argparse::default_arguments::none ),
        m_hasInputArg( false ),
        m_hasOutputArg( false ),
        m_hasDrawingSheetArg( false ),
        m_hasDefineArg( false ),
        m_outputArgExpectsDir( false ),
        m_hasVariantArg( false )

{
    m_argParser.add_argument( ARG_HELP_SHORT, ARG_HELP )
                .help( UTF8STDSTR( ARG_HELP_DESC ) )
                .flag()
                .nargs( 0 );
}


void CLI::COMMAND::PrintHelp()
{
    std::stringstream ss;
    ss << m_argParser;
    wxPrintf( From_UTF8( ss.str().c_str() ) );
}


int CLI::COMMAND::Perform( KIWAY& aKiway )
{
    if( m_argParser[ ARG_HELP ] == true )
    {
        PrintHelp();

        return 0;
    }

    if ( m_hasInputArg )
    {
        m_argInput = From_UTF8( m_argParser.get<std::string>( ARG_INPUT ).c_str() );
    }

    if( m_hasOutputArg )
    {
        m_argOutput = From_UTF8( m_argParser.get<std::string>( ARG_OUTPUT ).c_str() );
    }

    if( m_hasDrawingSheetArg )
    {
        m_argDrawingSheet = From_UTF8( m_argParser.get<std::string>( ARG_DRAWING_SHEET ).c_str() );
    }

    if( m_hasDefineArg )
    {
        auto defines = m_argParser.get<std::vector<std::string>>( ARG_DEFINE_VAR_LONG );

        for( const std::string& def : defines )
        {
            wxString      str = From_UTF8( def.c_str() );
            wxArrayString bits;
            wxStringSplit( str, bits, wxS( '=' ) );

            if( bits.Count() == 2 )
            {
                m_argDefineVars[bits[0]] = bits[1];
            }
            else
            {
                return EXIT_CODES::ERR_ARGS;
            }
        }
    }

    if( m_hasVariantArg && m_argParser.is_used( ARG_VARIANT ) )
    {
        auto variantNames = m_argParser.get<std::vector<std::string>>( ARG_VARIANT );

        for( const auto& name : variantNames )
            m_argVariantNames.push_back( From_UTF8( name ) );

        if( m_argVariantNames.size() > 1 && m_hasOutputArg && !m_argOutput.IsEmpty() )
        {
            if( !m_argOutput.Contains( wxS( "${VARIANT}" ) ) )
            {
                wxFprintf( stderr,
                           _( "When specifying multiple variants, the output path must contain "
                              "${VARIANT} to generate separate output files for each variant.\n" ) );
                return EXIT_CODES::ERR_ARGS;
            }
        }
    }

    return doPerform( aKiway );
}


int CLI::COMMAND::doPerform( KIWAY& aKiway )
{
    // default case if we aren't overloaded, just print the help
    PrintHelp();

    return EXIT_CODES::OK;
}


void CLI::COMMAND::addCommonArgs( bool aInput, bool aOutput, bool aInputCanBeDir,
                                  bool aOutputIsDir )
{
    m_hasInputArg = aInput;
    m_hasOutputArg = aOutput;
    m_outputArgExpectsDir = aOutputIsDir;

    if( aInput )
    {
        if( aInputCanBeDir )
        {
            m_argParser.add_argument( ARG_INPUT )
                    .help( UTF8STDSTR( _( "Input directory" ) ) )
                    .metavar( "INPUT_DIR" );
        }

        m_argParser.add_argument( ARG_INPUT )
                    .help( UTF8STDSTR( _( "Input file" ) ) )
                    .metavar( "INPUT_FILE" );
    }

    if( aOutput )
    {
        if( aOutputIsDir )
        {
            m_argParser.add_argument( "-o", ARG_OUTPUT )
                    .default_value( std::string() )
                    .help( UTF8STDSTR( _( "Output directory" ) ) )
                    .metavar( "OUTPUT_DIR" );
        }
        else
        {
            m_argParser.add_argument( "-o", ARG_OUTPUT )
                    .default_value( std::string() )
                    .help( UTF8STDSTR( _( "Output file" ) ) )
                    .metavar( "OUTPUT_FILE" );
        }
    }
}


void CLI::COMMAND::addDrawingSheetArg()
{
    m_hasDrawingSheetArg = true;

    m_argParser.add_argument( ARG_DRAWING_SHEET )
            .default_value( std::string() )
            .help( UTF8STDSTR( _( "Path to drawing sheet, this overrides any existing project "
                                  "defined sheet when used" ) ) )
            .metavar( "SHEET_PATH" );
}


void CLI::COMMAND::addDefineArg()
{
    m_hasDefineArg = true;

    m_argParser.add_argument( ARG_DEFINE_VAR_LONG, ARG_DEFINE_VAR_SHORT )
            .default_value( std::vector<std::string>() )
            .append()
            .help( UTF8STDSTR(
                    _( "Overrides or adds project variables, can be used multiple times to "
                       "declare multiple variables."
                       "\nUse in the format of '--define-var key=value' or '-D key=value'" ) ) )
            .metavar( "KEY=VALUE" );
}


void CLI::COMMAND::addVariantsArg()
{
    m_hasVariantArg = true;

    m_argParser.add_argument( ARG_VARIANT )
            .default_value( std::vector<std::string>() )
            .append()
            .help( UTF8STDSTR(
                    _( "The variant name(s) to output, can be used multiple times to specify "
                       "multiple variants.\n"
                       "When specifying multiple variants, use ${VARIANT} in the output path to "
                       "generate separate files for each variant.\n"
                       "When no --variant argument is provided the default variant is output." ) ) );
}
