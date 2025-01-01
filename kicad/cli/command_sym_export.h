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

#ifndef COMMAND_SYM_EXPORT_H
#define COMMAND_SYM_EXPORT_H

#include "command.h"

namespace CLI
{
struct SYM_EXPORT_COMMAND : public COMMAND
{
    SYM_EXPORT_COMMAND() : COMMAND( "export" )
    {
        m_argParser.add_description(
                UTF8STDSTR( _( "Export utilities (svg)" ) ) );
    }
};
}

#endif