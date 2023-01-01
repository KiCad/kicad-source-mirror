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

#include "command.h"
#include <cli/exit_codes.h>
#include <wx/crt.h>
#include <macros.h>

#include <sstream>


CLI::COMMAND::COMMAND( const std::string& aName ) :
        m_name( aName ),
        m_argParser( aName, "", argparse::default_arguments::none )
{
    m_argParser.add_argument( ARG_HELP_SHORT, ARG_HELP )
                .default_value( false )
                .help( UTF8STDSTR( ARG_HELP_DESC ) )
                .implicit_value( true )
                .nargs( 0 );
}


void CLI::COMMAND::PrintHelp()
{
    std::stringstream ss;
    ss << m_argParser;
    wxPrintf( FROM_UTF8( ss.str().c_str() ) );
}


int CLI::COMMAND::Perform( KIWAY& aKiway )
{
    if( m_argParser[ARG_HELP] == true )
    {
        PrintHelp();

        return 0;
    }

    return doPerform( aKiway );
}


int CLI::COMMAND::doPerform( KIWAY& aKiway )
{
    // default case if we aren't overloaded, just print the help
    PrintHelp();

    return EXIT_CODES::OK;
}