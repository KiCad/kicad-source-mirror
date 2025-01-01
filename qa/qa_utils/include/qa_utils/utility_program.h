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

#ifndef UTILITY_PROGRAM_H
#define UTILITY_PROGRAM_H

#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace KI_TEST
{

/**
 * Return codes for tools
 */
enum RET_CODES
{
    /// Tool exited OK
    OK = 0,

    /// The command line was not correct for the tool
    BAD_CMDLINE = 1,

    /// The tool asked for was not found
    UNKNOWN_TOOL = 2,

    /// Tools can define their own statuses from here onwards
    TOOL_SPECIFIC = 10,
};


/**
 * Description of a "utility program", which is a program that
 * takes some command line and does "something".
 *
 * Likely uses of this are:
 *
 * * Test programs and demos
 * * Benchmarks
 * * Fuzz targets
 *
 * This structure allows a single executable to select a program from
 * many registered programs to avoid having to maintain "N" similar CMake
 * scripts and perform "N" linkages.
 */
struct UTILITY_PROGRAM
{
    /// A function that provides the program for a given command line
    using FUNC = std::function<int( int argc, char** argv )>;

    UTILITY_PROGRAM( const std::string& aName, const std::string& aDesc, FUNC aMainFunc )
            : m_name( aName ), m_desc( aDesc ), m_func( aMainFunc )
    {
    }

    UTILITY_PROGRAM() : m_func( nullptr )
    {
    }

    /// The name of the program (this is used to select one)
    std::string m_name;

    /// Description of the program
    std::string m_desc;

    /// The function to call to run the program
    FUNC m_func;
};


/**
 * Class that handles delegation of command lines to one of a
 * number of "sub-utilities"
 */
class COMBINED_UTILITY
{
public:

    /**
     * Take in a command line and:
     *
     * * Handle "top level" commands like -h and -l
     * * Delegate to sub-utilities
     * * Report malformed command lines
     *
     * @param  argc argument count (directly from the main() parameter )
     * @param  argv argument values (directly from the main() parameter )
     * @return      return code
     */
    int HandleCommandLine( int argc, char** argv ) const;

private:
    /**
     * Format the list of known sub-utils.
     * @param os the stream to format on
     */
    void showSubUtilityList( std::ostream& os ) const;

    /**
     * Find a sub-utility with the given ID/name.
     * @param  aName the desired sub-utility name (e.g. "drc")
     * @return       pointer to the function that runs that sub-utility
     */
    UTILITY_PROGRAM::FUNC* findSubUtility( const std::string& aName ) const;

    /**
     * Print the command line usage of this program
     * @param name the name the program was run with
     * @param os   stream to print to
     */
    void printUsage( char* name, std::ostream& os ) const;
};

} // namespace KI_TEST

#endif // UTILITY_PROGRAM_H