/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file
 * Utility tool for parsing S-Expression data with the SEXPR class
 * for benchmarking, testing, etc.
 */

#include <sexpr/sexpr_parser.h>

#include <qa_utils/utility_registry.h>

#include <common.h>
#include <core/profile.h>

#include <wx/cmdline.h>

#include <fstream>
#include <iostream>


class QA_SEXPR_PARSER
{
public:
    QA_SEXPR_PARSER( bool aVerbose ) : m_verbose( aVerbose )
    {
    }

    bool Parse( std::istream& aStream )
    {
        // Don't let the parser handle stream reading - we don't want to
        // see how long the disk IO takes. Read to memory first (event the
        // biggest files will fit in)
        const std::string sexpr_str( std::istreambuf_iterator<char>( aStream ), {} );

        PROF_TIMER timer;
        // Perform the parse
        std::unique_ptr<SEXPR::SEXPR> sexpr( m_parser.Parse( sexpr_str ) );

        if( m_verbose )
            std::cout << "S-Expression Parsing took " << timer.msecs() << "ms" << std::endl;

        return sexpr != nullptr;
    }

private:
    bool          m_verbose;
    SEXPR::PARSER m_parser;
};

static const wxCmdLineEntryDesc g_cmdLineDesc[] = {
    {
            wxCMD_LINE_SWITCH,
            "h",
            "help",
            _( "displays help on the command line parameters" ).mb_str(),
            wxCMD_LINE_VAL_NONE,
            wxCMD_LINE_OPTION_HELP,
    },
    {
            wxCMD_LINE_SWITCH,
            "v",
            "verbose",
            _( "print parsing information" ).mb_str(),
    },
    {
            wxCMD_LINE_PARAM,
            nullptr,
            nullptr,
            _( "input file" ).mb_str(),
            wxCMD_LINE_VAL_STRING,
            wxCMD_LINE_PARAM_OPTIONAL | wxCMD_LINE_PARAM_MULTIPLE,
    },
    { wxCMD_LINE_NONE }
};


enum PARSER_RET_CODES
{
    PARSE_FAILED = KI_TEST::RET_CODES::TOOL_SPECIFIC,
};


int sexpr_parser_func( int argc, char* argv[] )
{
    wxCmdLineParser cl_parser( argc, argv );
    cl_parser.SetDesc( g_cmdLineDesc );
    cl_parser.AddUsageText( _( "Tests parsing of S-Expression files" ) );

    int cmd_parsed_ok = cl_parser.Parse();
    if( cmd_parsed_ok != 0 )
    {
        // Help and invalid input both stop here
        return ( cmd_parsed_ok == -1 ) ? KI_TEST::RET_CODES::OK : KI_TEST::RET_CODES::BAD_CMDLINE;
    }

    const auto file_count = cl_parser.GetParamCount();
    const bool verbose = cl_parser.Found( "verbose" );

    QA_SEXPR_PARSER qa_parser( verbose );

    bool ok = true;

    if( file_count == 0 )
    {
        // Parse the file provided on stdin - used by AFL to drive the
        // program
        qa_parser.Parse( std::cin );
    }
    else
    {
        // Parse 'n' files given on the command line
        // (this is useful for input minimisation (e.g. afl-tmin) as
        // well as manual testing
        for( unsigned i = 0; i < file_count; i++ )
        {
            const auto filename = cl_parser.GetParam( i ).ToStdString();

            if( verbose )
                std::cout << "Parsing: " << filename << std::endl;

            std::ifstream fin;
            fin.open( filename );

            ok = ok && qa_parser.Parse( fin );
        }
    }

    if( !ok )
        return PARSER_RET_CODES::PARSE_FAILED;

    return KI_TEST::RET_CODES::OK;
}


static bool registered = UTILITY_REGISTRY::Register( {
        "sexpr_parser",
        "Benchmark s-expression parsing",
        sexpr_parser_func,
} );
