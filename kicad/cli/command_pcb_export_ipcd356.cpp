/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2025 Connor Goss <connor.goss@acroname.com>
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

#include "command_pcb_export_ipcd356.h"
#include <cli/exit_codes.h>
#include "jobs/job_export_pcb_ipcd356.h"
#include <kiface_base.h>
#include <string_utils.h>
#include <wx/crt.h>

#include <locale_io.h>

CLI::PCB_EXPORT_IPCD356_COMMAND::PCB_EXPORT_IPCD356_COMMAND() :
        PCB_EXPORT_BASE_COMMAND( "ipcd356" )
{
    m_argParser.add_description( std::string( "Generate IPC-D-356 netlist file" ) );
}


int CLI::PCB_EXPORT_IPCD356_COMMAND::doPerform( KIWAY& aKiway )
{
    std::unique_ptr<JOB_EXPORT_PCB_IPCD356> ipcd356Job( new JOB_EXPORT_PCB_IPCD356() );

    ipcd356Job->m_filename = m_argInput;
    ipcd356Job->SetConfiguredOutputPath( m_argOutput );

    if( !wxFile::Exists( ipcd356Job->m_filename ) )
    {
        wxFprintf( stderr, _( "Board file does not exist or is not accessible\n" ) );
        return EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    return aKiway.ProcessJob( KIWAY::FACE_PCB, ipcd356Job.get() );
}
