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

#include "command_fp_upgrade.h"
#include <cli/exit_codes.h>
#include "jobs/job_fp_upgrade.h"
#include <kiface_base.h>
#include <layer_ids.h>
#include <wx/crt.h>
#include <wx/dir.h>

#include <macros.h>

#define ARG_FORCE "--force"

CLI::FP_UPGRADE_COMMAND::FP_UPGRADE_COMMAND() : EXPORT_PCB_BASE_COMMAND( "upgrade" )
{
    m_argParser.add_argument( ARG_FORCE )
            .help( UTF8STDSTR(
                    _( "Forces the footprint library to be resaved regardless of versioning" ) ) )
            .implicit_value( true )
            .default_value( false );
}


int CLI::FP_UPGRADE_COMMAND::doPerform( KIWAY& aKiway )
{
    std::unique_ptr<JOB_FP_UPGRADE> fpJob = std::make_unique<JOB_FP_UPGRADE>( true );

    fpJob->m_libraryPath = FROM_UTF8( m_argParser.get<std::string>( ARG_INPUT ).c_str() );
    fpJob->m_outputLibraryPath = FROM_UTF8( m_argParser.get<std::string>( ARG_OUTPUT ).c_str() );
    fpJob->m_force = m_argParser.get<bool>( ARG_FORCE );

    if( !wxDir::Exists( fpJob->m_libraryPath ) )
    {
        wxFprintf( stderr, _( "Footprint path does not exist or is not accessible\n" ) );
        return EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    int exitCode = aKiway.ProcessJob( KIWAY::FACE_PCB, fpJob.get() );

    return exitCode;
}
