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

#ifndef CLI_COMMAND_H
#define CLI_COMMAND_H

#include <argparse/argparse.hpp>
#include <kiway.h>

#define UTF8STDSTR( s ) ( std::string( s.utf8_str() ) )

#define ARG_VERSION "--version"
#define ARG_HELP "--help"
#define ARG_HELP_SHORT "-h"
#define ARG_HELP_DESC _( "shows help message and exits" )

namespace CLI
{

class COMMAND
{
public:
    /**
    * Define a new COMMAND instance
    *
    * @param aName The name of the command that is to be used in the cli interface
    */
    COMMAND( const std::string& aName );

    /**
    * Entry point to processing commands from args and doing work
    */
    int Perform( KIWAY& aKiway );

    virtual ~COMMAND() = default;

    argparse::ArgumentParser& GetArgParser() { return m_argParser; }
    const std::string&        GetName() const { return m_name; }

    void PrintHelp();
protected:
    /**
     * The internal handler that should be overloaded to implement command specific
     * processing and work.
     *
     * If not overloaded, the command will simply emit the help options by default
     */
    virtual int              doPerform( KIWAY& aKiway );

    std::string              m_name;
    argparse::ArgumentParser m_argParser;
};

}

#endif