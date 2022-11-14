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

#include "command_export_sch_bom.h"
#include <cli/exit_codes.h>
#include "jobs/job_export_sch_bom.h"
#include <kiface_base.h>
#include <layer_ids.h>
#include <wx/crt.h>

#include <macros.h>

#define ARG_FORMAT "--format"

CLI::EXPORT_SCH_BOM_COMMAND::EXPORT_SCH_BOM_COMMAND() : EXPORT_PCB_BASE_COMMAND( "bom" )
{
    m_argParser.add_argument( ARG_FORMAT )
            .default_value( std::string( "xml" ) )
            .help( UTF8STDSTR( _( "Bom output format, valid options: xml" ) ) );
}


int CLI::EXPORT_SCH_BOM_COMMAND::Perform( KIWAY& aKiway )
{
    std::unique_ptr<JOB_EXPORT_SCH_BOM> bomJob =
            std::make_unique<JOB_EXPORT_SCH_BOM>( true );

    bomJob->m_filename = FROM_UTF8( m_argParser.get<std::string>( ARG_INPUT ).c_str() );
    bomJob->m_outputFile = FROM_UTF8( m_argParser.get<std::string>( ARG_OUTPUT ).c_str() );

    if( !wxFile::Exists( bomJob->m_filename ) )
    {
        wxFprintf( stderr, _( "Schematic file does not exist or is not accessible\n" ) );
        return EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    wxString format = FROM_UTF8( m_argParser.get<std::string>( ARG_FORMAT ).c_str() );
    if( format == "xml" )
    {
        bomJob->format = JOB_EXPORT_SCH_BOM::FORMAT::XML;
    }
    else
    {
        wxFprintf( stderr, _( "Invalid format\n" ) );
        return EXIT_CODES::ERR_ARGS;
    }

    int exitCode = aKiway.ProcessJob( KIWAY::FACE_SCH, bomJob.get() );

    return exitCode;
}