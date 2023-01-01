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

#include "command_export_sch_pythonbom.h"
#include <cli/exit_codes.h>
#include "jobs/job_export_sch_pythonbom.h"
#include <kiface_base.h>
#include <layer_ids.h>
#include <wx/crt.h>

#include <macros.h>


CLI::EXPORT_SCH_PYTHONBOM_COMMAND::EXPORT_SCH_PYTHONBOM_COMMAND() :
        EXPORT_PCB_BASE_COMMAND( "python-bom" )
{
}


int CLI::EXPORT_SCH_PYTHONBOM_COMMAND::doPerform( KIWAY& aKiway )
{
    std::unique_ptr<JOB_EXPORT_SCH_PYTHONBOM> bomJob =
            std::make_unique<JOB_EXPORT_SCH_PYTHONBOM>( true );

    bomJob->m_filename = FROM_UTF8( m_argParser.get<std::string>( ARG_INPUT ).c_str() );
    bomJob->m_outputFile = FROM_UTF8( m_argParser.get<std::string>( ARG_OUTPUT ).c_str() );

    if( !wxFile::Exists( bomJob->m_filename ) )
    {
        wxFprintf( stderr, _( "Schematic file does not exist or is not accessible\n" ) );
        return EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }


    int exitCode = aKiway.ProcessJob( KIWAY::FACE_SCH, bomJob.get() );

    return exitCode;
}