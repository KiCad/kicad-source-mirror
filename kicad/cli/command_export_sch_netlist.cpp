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

#include "command_export_sch_netlist.h"
#include <cli/exit_codes.h>
#include "jobs/job_export_sch_netlist.h"
#include <kiface_base.h>
#include <layer_ids.h>
#include <wx/crt.h>

#include <macros.h>

#define ARG_FORMAT "--format"

CLI::EXPORT_SCH_NETLIST_COMMAND::EXPORT_SCH_NETLIST_COMMAND() : EXPORT_PCB_BASE_COMMAND( "netlist" )
{
    m_argParser.add_argument( ARG_FORMAT )
            .default_value( std::string( "kicadsexpr" ) )
            .help( UTF8STDSTR( _( "Netlist output format, valid options: kicadsexpr, kicadxml, cadstar, orcadpcb2, spice, spicemodel" ) ) );
}


int CLI::EXPORT_SCH_NETLIST_COMMAND::doPerform( KIWAY& aKiway )
{
    std::unique_ptr<JOB_EXPORT_SCH_NETLIST> netJob =
            std::make_unique<JOB_EXPORT_SCH_NETLIST>( true );

    netJob->m_filename = FROM_UTF8( m_argParser.get<std::string>( ARG_INPUT ).c_str() );
    netJob->m_outputFile = FROM_UTF8( m_argParser.get<std::string>( ARG_OUTPUT ).c_str() );

    if( !wxFile::Exists( netJob->m_filename ) )
    {
        wxFprintf( stderr, _( "Schematic file does not exist or is not accessible\n" ) );
        return EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    wxString format = FROM_UTF8( m_argParser.get<std::string>( ARG_FORMAT ).c_str() );
    if( format == "kicadsexpr" )
    {
        netJob->format = JOB_EXPORT_SCH_NETLIST::FORMAT::KICADSEXPR;
    }
    else if( format == "kicadxml" )
    {
        netJob->format = JOB_EXPORT_SCH_NETLIST::FORMAT::KICADXML;
    }
    else if( format == "cadstar" )
    {
        netJob->format = JOB_EXPORT_SCH_NETLIST::FORMAT::CADSTAR;
    }
    else if( format == "orcadpcb2" )
    {
        netJob->format = JOB_EXPORT_SCH_NETLIST::FORMAT::ORCADPCB2;
    }
    else if( format == "spice" )
    {
        netJob->format = JOB_EXPORT_SCH_NETLIST::FORMAT::SPICE;
    }
    else if( format == "spicemodel" )
    {
        netJob->format = JOB_EXPORT_SCH_NETLIST::FORMAT::SPICEMODEL;
    }
    else
    {
        wxFprintf( stderr, _( "Invalid format\n" ) );
        return EXIT_CODES::ERR_ARGS;
    }

    int exitCode = aKiway.ProcessJob( KIWAY::FACE_SCH, netJob.get() );

    return exitCode;
}