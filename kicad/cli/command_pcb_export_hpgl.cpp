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

#include "command_pcb_export_hpgl.h"
#include <cli/exit_codes.h>
#include <string_utils.h>
#include <wx/crt.h>


CLI::PCB_EXPORT_HPGL_COMMAND::PCB_EXPORT_HPGL_COMMAND() :
        PCB_EXPORT_BASE_COMMAND( "hpgl", false, true )
{
    m_argParser.add_description( UTF8STDSTR( _( "No longer supported as of KiCad 10.0." ) ) );
}


int CLI::PCB_EXPORT_HPGL_COMMAND::doPerform( KIWAY& aKiway )
{
    wxFprintf( stderr, _( "Plotting to HPGL is no longer supported as of KiCad 10.0.\n" ) );
    return EXIT_CODES::ERR_ARGS;
}
