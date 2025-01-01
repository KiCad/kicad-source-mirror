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
#include <qa_utils/utility_program.h>
#include <qa_utils/utility_registry.h>

#include <wx/msgout.h>

namespace KI_TEST
{

void COMBINED_UTILITY::showSubUtilityList( std::ostream& os ) const
{
    for( const auto& it : UTILITY_REGISTRY::GetInfoMap() )
    {
        const UTILITY_PROGRAM& prog = it.second;

        os << "Reg: " << prog.m_name << ": \t" << prog.m_desc << std::endl;
    }
}


UTILITY_PROGRAM::FUNC* COMBINED_UTILITY::findSubUtility( const std::string& aName ) const
{
    try
    {
        UTILITY_PROGRAM& prog = UTILITY_REGISTRY::GetInfoMap().at( aName );

        return &prog.m_func;
    }
    catch( const std::out_of_range& )
    {
        // not found in map
    }

    return nullptr;
}


void COMBINED_UTILITY::printUsage( char* name, std::ostream& os ) const
{
    os << "Run a utility tool." << std::endl;

    os << "Usage: " << name << " [-h] [-l] [TOOL [TOOL_OPTIONS]]" << std::endl;

    os << "  -h      show this message and exit." << std::endl
       << "  -l      print known tools and exit." << std::endl;

    os << std::endl;
    os << "Known tools: " << std::endl;

    showSubUtilityList( os );
}


int COMBINED_UTILITY::HandleCommandLine( int argc, char** argv ) const
{
    wxMessageOutput::Set( new wxMessageOutputStderr );

    // Need at least one parameter
    if( argc < 2 )
    {
        printUsage( argv[0], std::cerr );
        return RET_CODES::BAD_CMDLINE;
    }

    const std::string arg1( argv[1] );

    if( argc == 2 )
    {
        if( arg1 == "-h" )
        {
            printUsage( argv[0], std::cout );
            return RET_CODES::OK;
        }
        else if( arg1 == "-l" )
        {
            showSubUtilityList( std::cout );
            return RET_CODES::OK;
        }
    }

    auto func = findSubUtility( arg1 );

    if( !func )
    {
        return RET_CODES::UNKNOWN_TOOL;
    }

    // pass on the rest of the commands
    return ( *func )( argc - 1, argv + 1 );
}

} // namespace KI_TEST
