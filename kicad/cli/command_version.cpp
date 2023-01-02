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

#include "command_version.h"
#include <cli/exit_codes.h>
#include <wx/crt.h>
#include <kicad_build_version.h>


CLI::VERSION_COMMAND::VERSION_COMMAND() : COMMAND( "version" )
{
}


int CLI::VERSION_COMMAND::doPerform( KIWAY& aKiway )
{
    wxPrintf( KICAD_MAJOR_MINOR_PATCH_VERSION );

    return 0;
}