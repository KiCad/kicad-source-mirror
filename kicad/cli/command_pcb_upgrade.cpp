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

#include "command_pcb_upgrade.h"
#include "jobs/job_pcb_upgrade.h"
#include "cli/exit_codes.h"
#include <wx/crt.h>

#define ARG_FORCE "--force"

CLI::PCB_UPGRADE_COMMAND::PCB_UPGRADE_COMMAND() :
        COMMAND( "upgrade" )
{
    addCommonArgs( true, false, false, false );
    m_argParser.add_description( UTF8STDSTR( _( "Upgrade the board file's format to the latest one" ) ) );

    m_argParser.add_argument( ARG_FORCE )
            .help( UTF8STDSTR( _( "Forces the board file to be resaved regardless of versioning" ) ) )
            .flag();
}

int CLI::PCB_UPGRADE_COMMAND::doPerform( KIWAY& aKiway )
{
    std::unique_ptr<JOB_PCB_UPGRADE> upgradeJob = std::make_unique<JOB_PCB_UPGRADE>();

    upgradeJob->m_filename = m_argInput;
    upgradeJob->m_force = m_argParser.get<bool>( ARG_FORCE );

    if( !wxFile::Exists( upgradeJob->m_filename ) )
    {
        wxFprintf( stderr, _( "Board file does not exist or is not accessible\n" ) );
        return EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    int exitCode = aKiway.ProcessJob( KIWAY::FACE_PCB, upgradeJob.get() );

    return exitCode;
}
