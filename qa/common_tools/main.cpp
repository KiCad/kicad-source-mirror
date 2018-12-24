/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <common.h>

#include "tools/coroutines/coroutine_tools.h"

#include <wx/cmdline.h>

/**
 * List of registered tools.
 *
 * This is a pretty rudimentary way to register, but for a simple purpose,
 * it's effective enough. When you have a new tool, add it to this list.
 */
const static std::vector<UTILITY_PROGRAM*> known_tools = {
    &coroutine_tool,
};


/**
 * Print the names and descriptions of the registered tools
 */
static void show_tool_list()
{
    for( const auto& tool : known_tools )
    {
        std::cout << tool->m_name << ": \t" << tool->m_desc << std::endl;
    }
}


/**
 * Get the utility program that matches a tool name
 * @param  aName the name to look for
 * @return       the tool function
 */
UTILITY_PROGRAM::FUNC* get_program( const std::string& aName )
{
    for( const auto& tool : known_tools )
    {
        if( tool->m_name == aName )
            return &tool->m_func;
    }

    return nullptr;
}


void print_usage( char* name )
{
    std::cout << "Run a utility tool." << std::endl;

    std::cout << "Usage: " << name << " [-h] [-l] [TOOL [TOOL_OPTIONS]]" << std::endl;

    std::cout << "  -h      show this message and exit." << std::endl
              << "  -l      print known tools and exit." << std::endl;

    std::cout << std::endl;
    std::cout << "Known tools: " << std::endl;

    show_tool_list();
}


int main( int argc, char** argv )
{
    wxMessageOutput::Set( new wxMessageOutputStderr );

    // Need at least one parameter
    if( argc < 2 )
    {
        print_usage( argv[0] );
        return RET_CODES::BAD_CMDLINE;
    }

    const std::string arg1( argv[1] );

    if( argc == 2 )
    {
        if( arg1 == "-h" )
        {
            print_usage( argv[0] );
            return RET_CODES::OK;
        }
        else if( arg1 == "-l" )
        {
            show_tool_list();
            return RET_CODES::OK;
        }
    }

    auto func = get_program( arg1 );

    if( !func )
    {
        std::cout << "Tool " << arg1 << " not found." << std::endl;
        return RET_CODES::UNKNOWN_TOOL;
    }

    // pass on the rest of the commands
    return ( *func )( argc - 1, argv + 1 );
}