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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef COMMAND_GERBER_H
#define COMMAND_GERBER_H

#include "command.h"

namespace CLI
{

struct GERBER_COMMAND : public COMMAND
{
    GERBER_COMMAND() :
            COMMAND( "gerber" )
    {
        m_argParser.add_description( UTF8STDSTR( _( "View and compare existing Gerber/Excellon files. "
                                                    "To export Gerbers from a PCB, use 'pcb export gerbers'" ) ) );
    }
};

} // namespace CLI

#endif
