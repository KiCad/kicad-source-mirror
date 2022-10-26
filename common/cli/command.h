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

namespace CLI
{

class COMMAND
{
public:
    COMMAND( std::string aName ) : m_name( aName ), m_argParser( aName ){};

    virtual int Perform( KIWAY& aKiway ) const = 0;

    virtual ~COMMAND() = default;

    argparse::ArgumentParser& GetArgParser() { return m_argParser; }
    const std::string&        GetName() const { return m_name; }

protected:
    std::string              m_name;
    argparse::ArgumentParser m_argParser;
};

}

#endif